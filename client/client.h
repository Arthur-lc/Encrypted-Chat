#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <regex>

#include "UIManager.h"

class Client
{
private:
    int clientSocket;
    sockaddr_in serverAddress;
    std::atomic<bool> connected;
    std::thread receiverThread;
    UIManager& uiManager;
    std::string userName;

    void receiveMessages();
    void sendMessage(const std::string& msg);
    void handleMessage(const std::string& msg);
    void parseMessage(const std::string& msg, std::string& outSender, std::string& outMsg);

public:
    Client(const char *serverIp, int port, UIManager& uiManager);
    ~Client();

    bool connectToServer();
    void run();
    void stop();
};
