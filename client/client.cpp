#include "client.h"
#include <cstring>
#include <map>
// TESTES e PREGUIÇA
#include <cmath>

const int MESSAGE_BUFFER_SIZE = 4096;

Client::Client(const char *serverIp, int port, UIManager &ui) : uiManager(ui), connected(false)
{
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        uiManager.drawMessage("System", "Error creating socket", Color::Yellow);
        return;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    if (inet_pton(AF_INET, serverIp, &serverAddress.sin_addr) <= 0)
    {
        uiManager.drawMessage("System", "Invalid address or address not supported", Color::Yellow);
        close(clientSocket);
        clientSocket = -1;
    }

    privateKey = CryptoUtils::generatePrivateKey();
    publicKey = CryptoUtils::generatePublicKey(privateKey);
}

Client::~Client()
{
    stop();
}

void Client::stop()
{
    if (connected)
    {
        connected = false;
    }
    if (receiverThread.joinable())
    {
        receiverThread.join();
    }
    if (clientSocket >= 0)
    {
        close(clientSocket);
    }
}

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

bool Client::connectToServer()
{
    if (clientSocket < 0)
        return false;

    if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        uiManager.drawMessage("System", "Connection Failed", Color::Yellow);
        return false;
    }

    uiManager.updateStatus("Enter your name: ");
    username = uiManager.getUserInput();
    uiManager.clearInput();

    json j;
    j["type"] = "C2S_AUTHENTICATE_AND_JOIN";
    j["payload"]["username"] = username;
    j["payload"]["publicKey"] = publicKey;

    string jsonStr = j.dump();
    uiManager.debugLog(jsonStr);

    if (!sendAll(clientSocket, jsonStr.c_str(), jsonStr.size()))
    {
        uiManager.drawMessage("System", "Failed to send user name", Color::Yellow);
        return false;
    }

    connected = true;
    uiManager.updateStatus("Connected as: " + username);
    return true;
}

void Client::run()
{
    if (!connected)
        return;

    receiverThread = thread(&Client::receiveMessages, this);

    while (connected)
    {
        string msg = uiManager.getUserInput();
        sendMessage(msg);

        if (!connected)
            break;
    }
    stop();
}

void Client::receiveMessages()
{
    string jsonStr;
    while (connected)
    {
        if (!recvAll(clientSocket, jsonStr))
        {
            uiManager.drawMessage("System", "Server disconnected", Color::Yellow);
            connected = false;
            uiManager.updateStatus("Disconnected. Press any key to exit.");

            break;
        }

        try {
            if (jsonStr.empty()) {
                uiManager.drawMessage("System", "Empty message received", Color::Yellow);
                continue;
            }
            
            json j;
            try {
                j = json::parse(jsonStr);
            } catch (const json::parse_error& e) {
                uiManager.drawMessage("System", "JSON parse error: " + string(e.what()), Color::Red);
                uiManager.debugLog("Received string: '" + jsonStr + "'");
                continue;
            }
            
            if (!j.contains("type")) {
                uiManager.drawMessage("System", "Message missing type field", Color::Yellow);
                continue;
            }
            
            string type = j.at("type");

            if (type == "S2C_BROADCAST_GROUP_MESSAGE") {
                handleMessage(j);
            }
            else if (type == "S2C_USER_NOTIFICATION") {
                handleUserNotification(j);
            }
            else if (type == "S2C_GROUP_MEMBERS_LIST") {
                groupMembers.clear();
                auto members = j.at("payload").at("members");
                for (const auto& m : members) {
                    groupMembers.push_back({m.at("username"), m.at("publicKey")});
                }
                
                // Aguarda comando do servidor para iniciar troca de chaves
                uiManager.drawMessage("System", "Group members updated. Waiting for key exchange...", Color::Gray);
            }
            else if (type == "S2C_START_KEY_EXCHANGE_ROUND1") {
                // Rodada 1: Calcula valor intermediário
                uiManager.drawMessage("System", "Starting key exchange round 1...", Color::Gray);
                
                // Encontra índice do usuário atual
                int myIndex = 0;
                for (size_t i = 0; i < groupMembers.size(); ++i) {
                    if (groupMembers[i].id == username) {
                        myIndex = i;
                        break;
                    }
                }
                
                // Calcula valor intermediário
                const auto& before = groupMembers[(myIndex - 1 + groupMembers.size()) % groupMembers.size()];
                const auto& after = groupMembers[(myIndex + 1) % groupMembers.size()];
                ull intermediateValue = CryptoUtils::calculateIntermediateValue(privateKey, before, after);
                
                // Envia valor intermediário para o servidor
                json round1Msg;
                round1Msg["type"] = "C2S_INTERMEDIATE_VALUE";
                round1Msg["payload"]["intermediateValue"] = intermediateValue;
                
                string jsonStr = round1Msg.dump();
                if (!sendAll(clientSocket, jsonStr.c_str(), jsonStr.size())) {
                    uiManager.drawMessage("System", "Failed to send intermediate value", Color::Yellow);
                } else {
                    uiManager.drawMessage("System", "Intermediate value sent to server", Color::Gray);
                }
            }
            else if (type == "S2C_START_KEY_EXCHANGE_ROUND2") {
                // Rodada 2: Recebe todos os valores intermediários e calcula chave secreta
                uiManager.drawMessage("System", "Starting key exchange round 2...", Color::Gray);
                
                // Encontra índice do usuário atual
                int myIndex = 0;
                for (size_t i = 0; i < groupMembers.size(); ++i) {
                    if (groupMembers[i].id == username) {
                        myIndex = i;
                        break;
                    }
                }
                
                // Constrói lista de valores intermediários na ordem correta
                std::vector<ull> intermediateValues(groupMembers.size(), 0);
                auto intermediateData = j.at("payload").at("intermediateValues");
                
                for (const auto& data : intermediateData) {
                    string memberUsername = data.at("username");
                    ull memberValue = data.at("intermediateValue").get<ull>();
                    
                    // Encontra o índice correto deste membro
                    for (size_t i = 0; i < groupMembers.size(); ++i) {
                        if (groupMembers[i].id == memberUsername) {
                            intermediateValues[i] = memberValue;
                            break;
                        }
                    }
                }
                
                // Calcula chave secreta compartilhada
                sharedSecret = CryptoUtils::calculateSharedSecret(
                    privateKey, myIndex, groupMembers, intermediateValues
                );
                
                uiManager.drawMessage("System", "Shared secret calculated: " + to_string(sharedSecret), Color::Gray);
                
                // Notifica servidor que completou rodada 2
                json round2Msg;
                round2Msg["type"] = "C2S_ROUND2_COMPLETED";
                
                string jsonStr = round2Msg.dump();
                if (!sendAll(clientSocket, jsonStr.c_str(), jsonStr.size())) {
                    uiManager.drawMessage("System", "Failed to notify round 2 completion", Color::Yellow);
                }
            }
            else if (type == "S2C_KEY_EXCHANGE_COMPLETED") {
                uiManager.drawMessage("System", "Group key exchange completed successfully!", Color::Gray);
            }
            else if (type == "S2C_INDIVIDUAL_KEY_RESET") {
                // Gera nova chave individual quando usuário fica sozinho
                string message = j.at("payload").at("message");
                uiManager.drawMessage("System", message, Color::Yellow);
                
                // Gera nova chave secreta individual (mantém chaves privada/pública inalteradas)
                sharedSecret = CryptoUtils::generatePrivateKey();
                
                uiManager.drawMessage("System", "New individual key generated: " + to_string(sharedSecret), Color::Gray);
                uiManager.drawMessage("System", "Note: Messages will be encrypted with your new individual key", Color::Gray);
            }
            else {
                uiManager.debugLog("Error while receivingMessage\n\tType: " + type + " not defined");
            }
        } catch (const std::exception& e) {
            uiManager.drawMessage("Error while receivingMessage\n\tException:\n", e.what(), Color::Red);
        }

    }
}

void Client::handleMessage(const json& j) {
    string sender = j.at("payload").at("sender");
    string message = j.at("payload").at("ciphertext");

    string decryptedMessage = CryptoUtils::decryptMessage(message, sharedSecret);

    uiManager.drawMessage(sender, decryptedMessage, Color::Gray);
}

void Client::handleUserNotification(const json& j) {
    string eventName = j.at("payload").at("event");

    if (eventName == "USER_JOINED") {
        string username = j.at("payload").at("username");
        string welcomeMsg = "'" + username + "' has joined!";
        uiManager.drawMessage("system", welcomeMsg, Color::Yellow);
    }
    else if (eventName == "USER_DISCONNECTED") {
        string username = j.at("payload").at("username");
        string disconnectMsg = "'" + username + "' has left the chat.";
        uiManager.drawMessage("system", disconnectMsg, Color::Yellow);
    }
    else {
        uiManager.debugLog("Error while receivingMessage\n\tEvent: " + eventName + " not defined");
    }

}

void Client::sendMessage(const string& msg)
{
    if (!msg.empty())
    {
        uiManager.drawMessage("You", msg, Color::Gray);

        string encryptedMessage = CryptoUtils::encryptMessage(msg, sharedSecret);

        json j;
        j["type"] = "C2S_SEND_GROUP_MESSAGE";
        j["payload"]["ciphertext"] = encryptedMessage;

        string jsonStr = j.dump();

        if (!sendAll(clientSocket, jsonStr.c_str(), jsonStr.size()))
        {
            uiManager.drawMessage("System", "Failed to send message", Color::Yellow);
            connected = false;
        }
    }
}