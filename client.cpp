#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class Client
{
private:
    int clientSocket;
    sockaddr_in serverAddress;
    std::atomic<bool> connected;
    std::thread receiverThread;

    void receiveMessages()
    {
        char buffer[4096];
        while (connected)
        {
            memset(buffer, 0, sizeof(buffer));
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived <= 0)
            {
                if (connected)
                {
                    std::cout << "\n[Server disconnected]" << std::endl;
                    connected = false;
                }
                break;
            }
            std::cout << "\n"
                      << buffer << std::endl
                      << "> " << std::flush;
        }
    }

public:
    Client(const char *serverIp, int port) : connected(false)
    {
        // Create socket
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0)
        {
            std::cerr << "Error creating socket" << std::endl;
            return;
        }

        // Setup server address
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);
        if (inet_pton(AF_INET, serverIp, &serverAddress.sin_addr) <= 0)
        {
            std::cerr << "Invalid address/ Address not supported" << std::endl;
            close(clientSocket);
            clientSocket = -1;
            return;
        }
    }

    ~Client()
    {
        if (connected)
        {
            connected = false; // Signal receiver thread to stop
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

    void run()
    {
        if (clientSocket < 0)
        {
            return; // Socket was not created successfully
        }

        // Connect to the server
        if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
        {
            std::cerr << "Connection Failed" << std::endl;
            return;
        }

        std::string name;
        std::cout << "Seu nome: ";
        std::cin >> name;

        if (send(clientSocket, name.c_str(), name.size(), 0) < 0)
        {
            std::cerr << "Failed to establish connection" << std::endl;
            connected = false;
            return;
        }

        std::cout << "Connected to the server!" << std::endl;
        connected = true;

        // Start a thread to receive messages
        receiverThread = std::thread(&Client::receiveMessages, this);

        // Main loop to send messages
        std::string msg;
        std::cout << "> " << std::flush;
        while (connected)
        {
            std::getline(std::cin, msg);
            if (!connected)
            {
                break;
            }
            if (!msg.empty())
            {
                if (send(clientSocket, msg.c_str(), msg.size(), 0) < 0)
                {
                    std::cerr << "Failed to send message" << std::endl;
                    break;
                }
            }
            std::cout << "> " << std::flush;
        }

        std::cout << "Connection closed." << std::endl;
    }
};