#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
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

public:
    Client(const char *serverIp, int port, UIManager& uiManager);
    ~Client();

    bool connectToServer();
    void run();
    void stop();
};
