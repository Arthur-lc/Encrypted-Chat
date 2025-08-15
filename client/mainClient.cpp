#include "UIManager.h"
#include "client.h"
#include <iostream>

int main(int argc, char* argv[]) {
    const char* serverIp = "127.0.0.1";
    int port = 8080;
    
    if (argc >= 2) {
        serverIp = argv[1];
    }
    if (argc >= 3) {
        port = std::atoi(argv[2]);
    }
    
    try {
        UIManager ui;
        Client client(serverIp, port, ui);

        if (client.connectToServer()) {
            client.run();
        } else {
            std::cerr << "Failed to connect to server " << serverIp << ":" << port << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        // If ncurses fails to initialize, this will catch it.
        // We can't use the UI, so just print to stderr.
        fprintf(stderr, "Fatal Error: %s\n", e.what());
        return 1;
    }
    
    return 0;
}

