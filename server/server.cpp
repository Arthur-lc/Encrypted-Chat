#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

const int MAX_CLIENTS = 30;

class Server {
private:
    int serverSocket;
    sockaddr_in serverAddress;
    std::vector<std::thread> workerThreads;
    std::vector<std::atomic<int>> clientSockets;
    std::vector<std::string> clientNames;
    std::atomic<bool> isRunning;

    bool handleNewClient(int threadId) {
        int clientSocket = clientSockets[threadId].load();
        char nameBuffer[1024] = {0};

        int bytesReceived = recv(clientSocket, nameBuffer, sizeof(nameBuffer) - 1, 0);
        if (bytesReceived <= 0) {
            std::cout << "Client disconnected before sending name on thread " << threadId << std::endl;
            close(clientSocket);
            clientSockets[threadId] = -1;
            return false;
        }

        clientNames[threadId] = std::string(nameBuffer);

        std::string welcomeMsg = "'" + clientNames[threadId] + "' has joined the chat.";
        std::cout << "Client " << welcomeMsg << std::endl;
        broadcastMessage(welcomeMsg, clientSocket);

        return true;
    }

    void handleClient(int threadId) {
        while (isRunning) {
            int clientSocket = clientSockets[threadId].load();
            if (clientSocket != -1) {
                if (!handleNewClient(threadId)) {
                    continue; // Wait for a new connection on this thread
                }

                char buffer[4096];

                while (isRunning) {
                    memset(buffer, 0, sizeof(buffer));
                    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

                    if (bytesReceived <= 0) {
                        std::string disconnectMsg = "'" + clientNames[threadId] + "' has left the chat.";
                        std::cout << "Client " << disconnectMsg << std::endl;
                        close(clientSocket);
                        clientSockets[threadId] = -1;
                        clientNames[threadId] = "";
                        broadcastMessage(disconnectMsg, -1); // broadcast to all
                        break; // Exit inner loop to wait for a new connection
                    }

                    std::string message(buffer);
                    
                    // Verificar se é uma mensagem de troca de chaves
                    if (message.substr(0, 4) == "KEY:") {
                        std::cout << "Key exchange message from " << clientNames[threadId] << std::endl;
                        // Repassar mensagem de troca de chaves para todos os outros clientes
                        broadcastMessage(message, clientSocket);
                    } else {
                        std::string formattedMsg = clientNames[threadId] + ": " + message;
                        std::cout << formattedMsg << std::endl;
                        broadcastMessage(formattedMsg, clientSocket);
                    }
                }
            } else {
                // If no client, sleep briefly to avoid busy-waiting
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

    void broadcastMessage(const std::string& message, int senderSocket) {
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            int clientSocket = clientSockets[i].load();
            if (clientSocket != -1 && clientSocket != senderSocket) {
                send(clientSocket, message.c_str(), message.length(), 0);
            }
        }
    }

public:
    Server(int port) : clientSockets(MAX_CLIENTS), clientNames(MAX_CLIENTS), isRunning(true) {
        // Initialize client sockets to -1 (no client)
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            clientSockets[i] = -1;
        }

        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            std::cerr << "Error creating socket" << std::endl;
            isRunning = false;
            return;
        }

        int opt = 1;
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);
        serverAddress.sin_addr.s_addr = INADDR_ANY;

        if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
            std::cerr << "Error binding socket" << std::endl;
            close(serverSocket);
            isRunning = false;
            return;
        }

        if (listen(serverSocket, 5) < 0) {
            std::cerr << "Error listening on socket" << std::endl;
            close(serverSocket);
            isRunning = false;
            return;
        }

        std::cout << "Server started on port " << port << ". Waiting for connections..." << std::endl;
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
        std::cout << "Server shut down." << std::endl;
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
                if (isRunning) std::cerr << "Error accepting connection" << std::endl;
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
                std::string errorMsg = "Server is full!";
                send(clientSocket, errorMsg.c_str(), errorMsg.length(), 0);
                close(clientSocket);
            }
        }
    }
};