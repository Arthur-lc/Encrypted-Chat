#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <cstring>
#include <map>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include "server.h"

using namespace std;
using namespace nlohmann;

const int MAX_CLIENTS = 30;

using ull = unsigned long long int; 

struct User {
    string username;   
    ull publicKey;
    bool hasCalculatedIntermediate;  // Flag para controlar se já calculou valor intermediário
    ull intermediateValue;           // Valor intermediário calculado
};



class Server {
    private:
    int serverSocket;
    sockaddr_in serverAddress;
    vector<thread> workerThreads;
    vector<atomic<int>> clientSockets;
    vector<string> clientNames;
    vector<User> users;
    atomic<bool> isRunning;
    vector<GrupMember> groupMembers;
    bool keyExchangeInProgress;      // Flag para controlar se troca de chaves está em andamento
    int round1Completed;             // Contador de usuários que completaram rodada 1
    int round2Completed;             // Contador de usuários que completaram rodada 2

    // send all bytes in 'data' reliably
    // returns true on success, false on error
    bool sendAll(int sockfd, const void* data, size_t len) {
        const char* buf = static_cast<const char*>(data);
        size_t totalSent = 0;

        while (totalSent < len) {
            ssize_t sent = send(sockfd, buf + totalSent, len - totalSent, 0);
            if (sent <= 0) {
                return false; // error or connection closed
            }
            totalSent += sent;
        }

        // send the delimiter '\n'
        char delimiter = '\n';
        if (send(sockfd, &delimiter, 1, 0) != 1) {
            return false;
        }

        return true;
    }

    // Receives one full message terminated by '\n'
    // Returns false on error/connection closed
    bool recvAll(int sockfd, std::string &outMessage) {
        static std::map<int, std::string> buffers;  // buffer per socket
        char temp[1024];
        outMessage.clear();

        // Get or create buffer for this socket
        auto& buffer = buffers[sockfd];

        while (true) {
            // Check if we already have a full line in the buffer
            size_t pos = buffer.find('\n');
            if (pos != std::string::npos) {
                outMessage = buffer.substr(0, pos);  // extract message (without \n)
                buffer.erase(0, pos + 1);            // remove processed part
                return true;
            }

            // Need to read more data
            ssize_t bytesReceived = recv(sockfd, temp, sizeof(temp), 0);
            if (bytesReceived <= 0) {
                // Clean up buffer for this socket
                buffers.erase(sockfd);
                return false; // error or connection closed
            }

            buffer.append(temp, bytesReceived);
        }
    }


    bool handleNewClient(int threadId) {
        int clientSocket = clientSockets[threadId].load();
        string jsonStr;


        if (!recvAll(clientSocket, jsonStr)) {
            cout << "Client disconnected before sending name on thread " << threadId << endl;
            close(clientSocket);
            clientSockets[threadId] = -1;
            return false;
        }

        cout << "buffer " << jsonStr << endl << endl;
        
        // Verifica se o JSON está completo
        if (jsonStr.empty()) {
            cout << "Empty JSON string received" << endl;
            return false;
        }
        
        json j;
        try {
            j = json::parse(jsonStr);
        } catch (const json::parse_error& e) {
            cout << "JSON parse error: " << e.what() << endl;
            cout << "Received string: '" << jsonStr << "'" << endl;
            cout << "String length: " << jsonStr.length() << endl;
            return false;
        }

        try
        {
            if (!j.contains("type") || !j.contains("payload")) {
                cout << "Invalid JSON structure - missing required fields" << endl;
                cout << "JSON: " << j.dump() << endl;
                return false;
            }
            
            string type = j.at("type");
            if (type != "C2S_AUTHENTICATE_AND_JOIN") {
                cout << "Unexpected message type: " << type << endl;
                return false;
            }
            
            if (!j.at("payload").contains("username") || !j.at("payload").contains("publicKey")) {
                cout << "Invalid payload structure - missing username or publicKey" << endl;
                cout << "Payload: " << j.at("payload").dump() << endl;
                return false;
            }
            
            string username = j.at("payload").at("username");
            ull publicKey = j.at("payload").at("publicKey").get<ull>();
            
            // Salva o membro
            groupMembers.push_back({username, publicKey});

            users[threadId].username = username;
            users[threadId].publicKey = publicKey;
            users[threadId].hasCalculatedIntermediate = false;
            users[threadId].intermediateValue = 0;
            clientNames[threadId] = username;

            json welcomeMsg;
            welcomeMsg["type"] = "S2C_USER_NOTIFICATION";
            welcomeMsg["payload"]["event"] = "USER_JOINED";
            welcomeMsg["payload"]["username"] = username;

            cout << "Client " << welcomeMsg.dump() << endl;
            broadcastMessage(welcomeMsg.dump(), clientSocket);
            broadcastGroupMembersList();
            
            // Inicia nova troca de chaves quando um usuário entra
            if (groupMembers.size() > 1) {
                // Aguarda um pouco para garantir que todos os clientes receberam a lista atualizada
                this_thread::sleep_for(chrono::milliseconds(100));
                initiateKeyExchange();
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            throw e;
        }


        return true;
    }

    void initiateKeyExchange() {
        if (keyExchangeInProgress) {
            cout << "Key exchange already in progress, skipping..." << endl;
            return;
        }
        
        if (groupMembers.size() < 2) {
            cout << "Not enough group members for key exchange (need at least 2, got " 
                 << groupMembers.size() << ")" << endl;
            return;
        }
        
        cout << "Starting key exchange for " << groupMembers.size() << " members..." << endl;
        
        keyExchangeInProgress = true;
        round1Completed = 0;
        round2Completed = 0;
        
        // Reset flags para todos os usuários
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clientSockets[i] != -1) {
                users[i].hasCalculatedIntermediate = false;
                users[i].intermediateValue = 0;
            }
        }
        
        // Verifica se todos os usuários ativos estão na lista de membros
        int activeUsers = 0;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clientSockets[i] != -1 && !users[i].username.empty()) {
                activeUsers++;
            }
        }
        
        cout << "Active users: " << activeUsers << ", Group members: " << groupMembers.size() << endl;
        
        // Envia comando para iniciar rodada 1
        json round1Msg;
        round1Msg["type"] = "S2C_START_KEY_EXCHANGE_ROUND1";
        round1Msg["payload"]["groupSize"] = groupMembers.size();
        broadcastMessage(round1Msg.dump(), -1);
    }

    void handleKeyExchangeRound1(int threadId, ull intermediateValue) {
        // Verifica se o threadId é válido
        if (threadId < 0 || threadId >= MAX_CLIENTS) {
            cout << "Invalid threadId in handleKeyExchangeRound1: " << threadId << endl;
            return;
        }
        
        // Verifica se o usuário ainda está ativo e conectado
        if (clientSockets[threadId] == -1 || users[threadId].username.empty()) {
            cout << "User on thread " << threadId << " is no longer active, skipping..." << endl;
            return;
        }
        
        // Verifica se o usuário ainda está na lista de membros do grupo
        bool userStillInGroup = false;
        for (const auto& member : groupMembers) {
            if (member.username == users[threadId].username) {
                userStillInGroup = true;
                break;
            }
        }
        
        if (!userStillInGroup) {
            cout << "User " << users[threadId].username << " is no longer in group, skipping..." << endl;
            return;
        }
        
        users[threadId].intermediateValue = intermediateValue;
        users[threadId].hasCalculatedIntermediate = true;
        round1Completed++;
        
        cout << "User " << users[threadId].username << " completed round 1. Progress: " 
             << round1Completed << "/" << groupMembers.size() << endl;
        
        // Se todos completaram rodada 1, inicia rodada 2
        if (round1Completed >= groupMembers.size()) {
            startRound2();
        }
    }

    void startRound2() {
        // Verifica se ainda há usuários suficientes para continuar
        if (groupMembers.size() < 2) {
            cout << "Not enough users for round 2, aborting key exchange" << endl;
            keyExchangeInProgress = false;
            return;
        }
        
        // Envia todos os valores intermediários para todos os clientes
        json round2Msg;
        round2Msg["type"] = "S2C_START_KEY_EXCHANGE_ROUND2";
        
        int validUsers = 0;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clientSockets[i] != -1 && users[i].hasCalculatedIntermediate && !users[i].username.empty()) {
                // Verifica se o usuário ainda está na lista de membros
                bool userStillInGroup = false;
                for (const auto& member : groupMembers) {
                    if (member.username == users[i].username) {
                        userStillInGroup = true;
                        break;
                    }
                }
                
                if (userStillInGroup) {
                    json member;
                    member["username"] = users[i].username;
                    member["intermediateValue"] = users[i].intermediateValue;
                    round2Msg["payload"]["intermediateValues"].push_back(member);
                    validUsers++;
                }
            }
        }
        
        cout << "Round 2: " << validUsers << " valid users out of " << groupMembers.size() << " group members" << endl;
        
        if (validUsers < groupMembers.size()) {
            cout << "Some users are no longer valid, restarting key exchange" << endl;
            keyExchangeInProgress = false;
            round1Completed = 0;
            round2Completed = 0;
            // Inicia nova troca de chaves
            this_thread::sleep_for(chrono::milliseconds(100));
            initiateKeyExchange();
            return;
        }
        
        broadcastMessage(round2Msg.dump(), -1);
    }

    void handleKeyExchangeRound2(int threadId) {
        // Verifica se o threadId é válido
        if (threadId < 0 || threadId >= MAX_CLIENTS) {
            cout << "Invalid threadId in handleKeyExchangeRound2: " << threadId << endl;
            return;
        }
        
        // Verifica se o usuário ainda está ativo e conectado
        if (clientSockets[threadId] == -1 || users[threadId].username.empty()) {
            cout << "User on thread " << threadId << " is no longer active, skipping..." << endl;
            return;
        }
        
        // Verifica se o usuário ainda está na lista de membros do grupo
        bool userStillInGroup = false;
        for (const auto& member : groupMembers) {
            if (member.username == users[threadId].username) {
                userStillInGroup = true;
                break;
            }
        }
        
        if (!userStillInGroup) {
            cout << "User " << users[threadId].username << " is no longer in group, skipping..." << endl;
            return;
        }
        
        round2Completed++;
        cout << "User " << users[threadId].username << " completed round 2. Progress: " 
             << round2Completed << "/" << groupMembers.size() << endl;
        
        // Se todos completaram rodada 2, finaliza troca de chaves
        if (round2Completed >= groupMembers.size()) {
            finalizeKeyExchange();
        }
    }

    void finalizeKeyExchange() {
        keyExchangeInProgress = false;
        cout << "Key exchange completed for all users!" << endl;
        
        // Notifica todos que a troca de chaves foi concluída
        json finalMsg;
        finalMsg["type"] = "S2C_KEY_EXCHANGE_COMPLETED";
        broadcastMessage(finalMsg.dump(), -1);
    }
    
    void cleanupInactiveUsers() {
        // Remove usuários que não estão mais ativos da lista de membros
        auto it = groupMembers.begin();
        while (it != groupMembers.end()) {
            bool userStillActive = false;
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (clientSockets[i] != -1 && users[i].username == it->username) {
                    userStillActive = true;
                    break;
                }
            }
            
            if (!userStillActive) {
                cout << "Removing inactive user " << it->username << " from group members" << endl;
                it = groupMembers.erase(it);
            } else {
                ++it;
            }
        }
        
        // Se após limpeza resta apenas 1 usuário, envia comando para chave individual
        if (groupMembers.size() == 1) {
            cout << "After cleanup: only 1 user remaining. Sending individual key command." << endl;
            json individualKeyMsg;
            individualKeyMsg["type"] = "S2C_INDIVIDUAL_KEY_RESET";
            individualKeyMsg["payload"]["message"] = "Other users left. You are now alone. Generating new individual key.";
            broadcastMessage(individualKeyMsg.dump(), -1);
        }
    }

    void handleClient(int threadId) {
        while (isRunning) {
            int clientSocket = clientSockets[threadId].load();
            if (clientSocket != -1) {
                if (!handleNewClient(threadId)) {
                    continue; // Wait for a new connection on this thread
                }

                string jsonStr;

                while (isRunning) {
                    cout << "recebi" << endl;
                    if (!recvAll(clientSocket, jsonStr)) {
                        json disconnectMsg;
                        disconnectMsg["type"] = "S2C_USER_NOTIFICATION";
                        disconnectMsg["payload"]["event"] = "USER_DISCONNECTED";
                        disconnectMsg["payload"]["username"] = users[threadId].username;
                        cout << "Client " << disconnectMsg << endl;

                        // Remove usuário da lista de membros do grupo
                        for (auto it = groupMembers.begin(); it != groupMembers.end(); ++it) {
                            if (it->username == users[threadId].username) {
                                groupMembers.erase(it);
                                break;
                            }
                        }
                        
                        // Reseta completamente a troca de chaves quando um usuário desconecta
                        if (keyExchangeInProgress) {
                            cout << "User " << users[threadId].username << " disconnected during key exchange. Restarting..." << endl;
                            keyExchangeInProgress = false;
                            round1Completed = 0;
                            round2Completed = 0;
                        }
                        
                        close(clientSocket);
                        clientSockets[threadId] = -1;
                        clientNames[threadId] = "";
                        users[threadId].username = "";
                        users[threadId].publicKey = 0;
                        users[threadId].hasCalculatedIntermediate = false;
                        users[threadId].intermediateValue = 0;
                        
                        broadcastMessage(disconnectMsg.dump(), -1); // broadcast to all
                        broadcastGroupMembersList(); // Atualiza lista de membros
                        
                        // Limpa usuários inativos antes de iniciar nova troca de chaves
                        cleanupInactiveUsers();
                        
                        // Inicia nova troca de chaves se ainda há usuários suficientes
                        if (groupMembers.size() >= 2) {
                            cout << "Starting new key exchange after user disconnect. Members: " << groupMembers.size() << endl;
                            // Aguarda um pouco para garantir que todos os clientes receberam a lista atualizada
                            this_thread::sleep_for(chrono::milliseconds(100));
                            initiateKeyExchange();
                        } else if (groupMembers.size() == 1) {
                            // Apenas 1 usuário restante, envia comando para gerar chave individual
                            cout << "Only 1 user remaining. Sending individual key reset command. Members: " << groupMembers.size() << endl;
                            json individualKeyMsg;
                            individualKeyMsg["type"] = "S2C_INDIVIDUAL_KEY_RESET";
                            individualKeyMsg["payload"]["message"] = "You are now alone. Generating new individual key.";
                            broadcastMessage(individualKeyMsg.dump(), -1);
                        } else {
                            cout << "No users remaining after disconnect. Members: " << groupMembers.size() << endl;
                        }
                        
                        break; // Exit inner loop to wait for a new connection
                    }

                    json j;
                    try {
                        j = json::parse(jsonStr);
                    } catch (const json::parse_error& e) {
                        cout << "JSON parse error in handleClient: " << e.what() << endl;
                        cout << "Received string: '" << jsonStr << "'" << endl;
                        cout << "String length: " << jsonStr.length() << endl;
                        continue; // Skip this message and continue
                    }

                    if (!j.contains("type")) {
                        cout << "Message missing type field: " << jsonStr << endl;
                        continue;
                    }
                    
                    string type = j.at("type");

                    if (type == "C2S_SEND_GROUP_MESSAGE") {
                        json newJ;
                        newJ["type"] = "S2C_BROADCAST_GROUP_MESSAGE";
                        newJ["payload"]["sender"] = users[threadId].username;
                        newJ["payload"]["ciphertext"] = j.at("payload").at("ciphertext");

                        string newJasonStr = newJ.dump();

                        cout << newJasonStr << endl;
                        
                        broadcastMessage(newJasonStr, clientSocket);
                        
                    } else if (type == "C2S_INTERMEDIATE_VALUE") {
                        // Cliente enviou seu valor intermediário (rodada 1)
                        try {
                            ull intermediateValue = j.at("payload").at("intermediateValue").get<ull>();
                            handleKeyExchangeRound1(threadId, intermediateValue);
                        } catch (const std::exception& e) {
                            cout << "Error parsing intermediate value from user " << users[threadId].username 
                                 << ": " << e.what() << endl;
                        }
                        
                    } else if (type == "C2S_ROUND2_COMPLETED") {
                        // Cliente completou rodada 2
                        handleKeyExchangeRound2(threadId);
                    }
                }
            } else {
                // If no client, sleep briefly to avoid busy-waiting
                this_thread::sleep_for(chrono::milliseconds(100));
            }
        }
    }

    void broadcastMessage(const string& message, int senderSocket) {
        cout << "Broadcasting message: " << message << endl;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            int clientSocket = clientSockets[i].load();
            if (clientSocket != -1 && clientSocket != senderSocket) {
                if (!sendAll(clientSocket, message.c_str(), message.length())) {
                    cout << "Failed to send message to client " << i << " (socket " << clientSocket << ")" << endl;
                }
            }
        }
    }
    void broadcastGroupMembersList() {
        nlohmann::json j;
        j["type"] = "S2C_GROUP_MEMBERS_LIST";
        for (const auto& member : groupMembers) {
            nlohmann::json m;
            m["username"] = member.username;
            m["publicKey"] = member.publicKey;
            j["payload"]["members"].push_back(m);
        }
        std::string msg = j.dump();
        cout << "Broadcasting group members list: " << msg << endl;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            int clientSock = clientSockets[i].load();
            if (clientSock != -1) {
                if (!sendAll(clientSock, msg.c_str(), msg.size())) {
                    cout << "Failed to send group members list to client " << i << " (socket " << clientSock << ")" << endl;
                }
            }
        }
    }

public:
    Server(int port) : clientSockets(MAX_CLIENTS), clientNames(MAX_CLIENTS), users(MAX_CLIENTS), 
                       isRunning(true), keyExchangeInProgress(false), round1Completed(0), round2Completed(0) {
        // Initialize client sockets to -1 (no client)
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            clientSockets[i] = -1;
            // Inicializa estrutura User com valores padrão
            users[i].username = "";
            users[i].publicKey = 0;
            users[i].hasCalculatedIntermediate = false;
            users[i].intermediateValue = 0;
        }

        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            cerr << "Error creating socket" << endl;
            isRunning = false;
            return;
        }

        int opt = 1;
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);
        serverAddress.sin_addr.s_addr = INADDR_ANY;

        if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
            cerr << "Error binding socket" << endl;
            close(serverSocket);
            isRunning = false;
            return;
        }

        if (listen(serverSocket, 5) < 0) {
            cerr << "Error listening on socket" << endl;
            close(serverSocket);
            isRunning = false;
            return;
        }

        cout << "Server started on port " << port << ". Waiting for connections..." << endl;
    }

    ~Server() {
        isRunning = false; // Signal all threads to stop
        for (auto& th : workerThreads) {
            if (th.joinable()) {
                th.join();
            }
        }
        for(int i = 0; i < MAX_CLIENTS; ++i) {
            if(clientSockets[i] != -1) {
                close(clientSockets[i]);
            }
        }
        close(serverSocket);
        cout << "Server shut down." << endl;
    }

    // Starts the server
    void run() {
        if (!isRunning) return;

        // Create worker threads
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            workerThreads.emplace_back(&Server::handleClient, this, i);
        }

        // Main loop to accept new connections
        while (isRunning) {
            int clientSocket = accept(serverSocket, nullptr, nullptr);
            if (clientSocket < 0) {
                if (isRunning) cerr << "Error accepting connection" << endl;
                continue;
            }

            bool assigned = false;
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (clientSockets[i] == -1) {
                    clientSockets[i] = clientSocket;
                    assigned = true;
                    break;
                }
            }

            if (!assigned) {
                string errorMsg = "Server is full!";
                sendAll(clientSocket, errorMsg.c_str(), errorMsg.length());
                close(clientSocket);
            }
        }
    }
};