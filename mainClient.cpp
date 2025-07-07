#include "UIManager.h"
#include "client.h"

int main() {
    try {
        UIManager ui;
        Client client("127.0.0.1", 8080, ui);

        if (client.connectToServer()) {
            client.run();
        }
    } catch (const std::exception& e) {
        // If ncurses fails to initialize, this will catch it.
        // We can't use the UI, so just print to stderr.
        fprintf(stderr, "Fatal Error: %s\n", e.what());
        return 1;
    }
    
    return 0;
}

