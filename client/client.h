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
#include "diffiehellman.h"
#include <nlohmann/json.hpp>

using namespace std;
using namespace nlohmann;

class Client
{
private:
    int clientSocket;
    sockaddr_in serverAddress;
    atomic<bool> connected;
    thread receiverThread;
    UIManager& uiManager;
    string username;

    ull privateKey;
    ull publicKey;

    void receiveMessages();
    void sendMessage(const string& msg);
    void handleMessage(const json& j);
    void handleUserNotification(const json& j);
    void parseMessage(const string& msg, string& outSender, string& outMsg);

public:
    Client(const char *serverIp, int port, UIManager& uiManager);
    ~Client();

    bool connectToServer();
    void run();
    void stop();
};
