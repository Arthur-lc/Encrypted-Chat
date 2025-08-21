#pragma once

#include <string>
#include <vector>
#include <ncurses.h>
#include <fstream>
#include <sstream>
#include <filesystem>

enum class Color {
    Gray = 1,
    Red = 2,
    Yellow = 3
};

class UIManager {
public:
    UIManager();
    ~UIManager();

    void drawMessage(const std::string& sender, const std::string& message, Color color);
    std::string getUserInput();
    void updateStatus(const std::string& status);
    void clearInput();
    void refreshAll();

    void debugLog(const std::string& log); // use to debug
    void writeDebugToFile(const std::string &log);

private:
    WINDOW *status_window;
    WINDOW *message_window;
    WINDOW *input_window;

    void drawBorders();
};