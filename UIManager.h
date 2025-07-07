#pragma once

#include <string>
#include <vector>
#include <ncurses.h>

class UIManager {
public:
    UIManager();
    ~UIManager();

    void drawMessage(const std::string& sender, const std::string& message);
    std::string getUserInput();
    void updateStatus(const std::string& status);
    void clearInput();
    void refreshAll();

private:
    WINDOW *status_window;
    WINDOW *message_window;
    WINDOW *input_window;

    std::vector<std::string> messages;
    int max_messages;

    void drawBorders();
};
