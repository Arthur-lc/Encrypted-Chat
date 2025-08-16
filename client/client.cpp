#include "client.h"
#include <cstring>

const int MESSAGE_BUFFER_SIZE = 4096;

Client::Client(const char *serverIp, int port, UIManager &ui) : uiManager(ui), connected(false), keysExchanged(false)
{
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        uiManager.drawMessage("System", "Error creating socket");
        return;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    if (inet_pton(AF_INET, serverIp, &serverAddress.sin_addr) <= 0)
    {
        uiManager.drawMessage("System", "Invalid address or address not supported");
        close(clientSocket);
        clientSocket = -1;
    }
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

bool Client::connectToServer()
{
    if (clientSocket < 0)
        return false;

    if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        uiManager.drawMessage("System", "Connection Failed");
        return false;
    }

    uiManager.updateStatus("Enter your name: ");
    userName = uiManager.getUserInput();
    uiManager.clearInput();

    if (send(clientSocket, userName.c_str(), userName.size(), 0) < 0)
    {
        uiManager.drawMessage("System", "Failed to send user name");
        return false;
    }

    connected = true;
    uiManager.updateStatus("Connected as: " + userName);
    
    // Iniciar troca de chaves
    exchangeKeys();
    
    return true;
}

void Client::exchangeKeys()
{
    sendPublicKey();
    uiManager.drawMessage("System", "Exchanging encryption keys...");
}

void Client::sendPublicKey()
{
    std::string publicKey = dh.getPublicKeyString();
    if (!publicKey.empty()) {
        std::string keyMsg = "KEY:" + userName + ":" + publicKey;
        send(clientSocket, keyMsg.c_str(), keyMsg.length(), 0);
    }
}

void Client::handleKeyExchange(const std::string& sender, const std::string& keyData)
{
    if (sender != userName) {
        // Verificar se já temos essa chave para evitar duplicatas
        bool keyExists = false;
        for (const auto& existingKey : otherPublicKeys) {
            if (existingKey == keyData) {
                keyExists = true;
                break;
            }
        }
        
        if (!keyExists) {
            otherPublicKeys.push_back(keyData);
            uiManager.drawMessage("System", "Received public key from " + sender);
            
            // Se temos pelo menos uma chave, estabelecer a chave de grupo
            if (otherPublicKeys.size() >= 1 && !keysExchanged) {
                dh.establishGroupKey(otherPublicKeys);
                keysExchanged = true;
                uiManager.drawMessage("System", "Group encryption key established!");
            }
        }
    }
}

void Client::run()
{
    if (!connected)
        return;

    receiverThread = std::thread(&Client::receiveMessages, this);

    while (connected)
    {
        std::string msg = uiManager.getUserInput();
        sendMessage(msg);

        if (!connected)
            break;
    }
    stop();
}

void Client::receiveMessages()
{
    char buffer[MESSAGE_BUFFER_SIZE];
    while (connected)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0)
        {
            if (connected)
            {
                uiManager.drawMessage("System", "Server disconnected");
                connected = false;
                uiManager.updateStatus("Disconnected. Press any key to exit.");
            }
            break;
        }

        // TODO: received pode conter mais de uma msg, precisamos implementar uma maneira de separar.
        std::string received(buffer); 
        handleMessage(received);        
    }
}

void Client::handleMessage(const std::string& received)
{
    std::string sender;
    std::string msg;

    parseMessage(received, sender, msg);
    
    // Verificar se é uma mensagem de troca de chaves
    if (msg.substr(0, 4) == "KEY:") {
        size_t firstColon = msg.find(':', 4);
        if (firstColon != std::string::npos) {
            std::string keySender = msg.substr(4, firstColon - 4);
            std::string keyData = msg.substr(firstColon + 1);
            handleKeyExchange(keySender, keyData);
            return;
        }
    }
    
    // Decriptar mensagem se as chaves foram trocadas
    if (keysExchanged && dh.isKeyEstablished()) {
        std::string decrypted = dh.decryptMessage(msg);
        uiManager.drawMessage(sender, decrypted);
    } else {
        uiManager.drawMessage(sender, msg);
    }
}

void Client::parseMessage(const std::string &msg, std::string &outSender, std::string &outMsg)
{
    size_t separator_pos = msg.find(':');
    if (separator_pos != std::string::npos)
    {
        outSender = msg.substr(0, separator_pos);
        outMsg = msg.substr(separator_pos + 1);
    }
    else
    {
        outMsg = msg;
        outSender = "Server";
    }
}

void Client::sendMessage(const std::string& msg)
{
    if (!msg.empty())
    {
        std::string messageToSend = msg;
        
        // Criptografar mensagem se as chaves foram trocadas
        if (keysExchanged && dh.isKeyEstablished()) {
            messageToSend = dh.encryptMessage(msg);
        }
        
        uiManager.drawMessage("You", msg);
        if (send(clientSocket, messageToSend.c_str(), messageToSend.length(), 0) < 0)
        {
            uiManager.drawMessage("System", "Failed to send message");
            connected = false;
        }
    }
}