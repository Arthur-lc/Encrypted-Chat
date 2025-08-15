#include "server.cpp"
#include <iostream>

int main(int argc, char* argv[])
{
    int port = 8080;
    
    if (argc >= 2) {
        port = std::atoi(argv[1]);
    }
    
    std::cout << "Starting server on port " << port << std::endl;
    Server server(port);
    server.run();
    return 0;
}
