#include "client.h"
#include <cstring>

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
            json j = json::parse(jsonStr);
            string type = j.at("type");

            if (type == "S2C_BROADCAST_GROUP_MESSAGE") {
                handleMessage(j);
            }
            else if (type == "S2C_USER_NOTIFICATION") {
                handleUserNotification(j);
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

    uiManager.drawMessage(sender, message, Color::Gray);
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

        json j;
        j["type"] = "C2S_SEND_GROUP_MESSAGE";
        j["payload"]["ciphertext"] = msg;

        string jsonStr = j.dump();

        if (!sendAll(clientSocket, jsonStr.c_str(), jsonStr.size()))
        {
            uiManager.drawMessage("System", "Failed to send message", Color::Yellow);
            connected = false;
        }
    }
}