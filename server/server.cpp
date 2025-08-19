#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <cstring>
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
        static std::string buffer;  // persistent across calls (per socket)
        char temp[1024];
        outMessage.clear();

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
        json j = json::parse(jsonStr);

        try
        {
            string type = j.at("type");
            string username = j.at("payload").at("username");
            ull publicKey = j.at("payload").at("publicKey").get<ull>();
            
            // Salva o membro
            groupMembers.push_back({username, publicKey});

            users[threadId].username = username;
            users[threadId].publicKey = publicKey;
            clientNames[threadId] = username;

            json welcomeMsg;
            welcomeMsg["type"] = "S2C_USER_NOTIFICATION";
            welcomeMsg["payload"]["event"] = "USER_JOINED";
            welcomeMsg["payload"]["username"] = username;

            cout << "Client " << welcomeMsg.dump() << endl;
            broadcastMessage(welcomeMsg.dump(), clientSocket);
            broadcastGroupMembersList();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            throw e;
        }


        return true;
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

                        close(clientSocket);
                        clientSockets[threadId] = -1;
                        clientNames[threadId] = "";
                        broadcastMessage(disconnectMsg.dump(), -1); // broadcast to all
                        break; // Exit inner loop to wait for a new connection
                    }

                    json j = json::parse(jsonStr);

                    string type = j.at("type");

                    if (type == "C2S_SEND_GROUP_MESSAGE") {
                        json newJ;
                        newJ["type"] = "S2C_BROADCAST_GROUP_MESSAGE";
                        newJ["payload"]["sender"] = users[threadId].username;
                        newJ["payload"]["ciphertext"] = j.at("payload").at("ciphertext");

                        string newJasonStr = newJ.dump();

                        cout << newJasonStr << endl;
                        
                        broadcastMessage(newJasonStr, clientSocket);
                        
                    }
                }
            } else {
                // If no client, sleep briefly to avoid busy-waiting
                this_thread::sleep_for(chrono::milliseconds(100));
            }
        }
    }

    void broadcastMessage(const string& message, int senderSocket) {
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            int clientSocket = clientSockets[i].load();
            if (clientSocket != -1 && clientSocket != senderSocket) {
                sendAll(clientSocket, message.c_str(), message.length());
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
        for (int clientSock : clientSockets) {
            sendAll(clientSock, msg.c_str(), msg.size());
        }
    }

public:
    Server(int port) : clientSockets(MAX_CLIENTS), clientNames(MAX_CLIENTS), users(MAX_CLIENTS), isRunning(true) {
        // Initialize client sockets to -1 (no client)
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            clientSockets[i] = -1;
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

    /*Starts the server*/
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