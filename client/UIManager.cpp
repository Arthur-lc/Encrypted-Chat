#include "UIManager.h"
#include <stdexcept>

UIManager::UIManager() {
    initscr();
    if (stdscr == NULL) {
        throw std::runtime_error("Error initializing ncurses.");
    }

    // Basic settings
    //cbreak();
    keypad(stdscr, TRUE);
    //curs_set(1);

    start_color(); // Inicia as cores
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);

    // Create windows
    int height, width;
    getmaxyx(stdscr, height, width);

    status_window = newwin(1, width, 0, 0);
    message_window = newwin(height - 2, width, 1, 0);
    input_window = newwin(1, width, height - 1, 0);

    if (!status_window || !message_window || !input_window) {
        endwin();
        throw std::runtime_error("Failed to create windows.");
    }

    scrollok(message_window, TRUE);
    scrollok(input_window, TRUE);

    keypad(input_window, TRUE);

    updateStatus("Connecting...");
    refreshAll();
}

UIManager::~UIManager() {
    // Cleanup
    delwin(status_window);
    delwin(message_window);
    delwin(input_window);
    endwin();
}

void UIManager::updateStatus(const std::string& status) {
    wclear(status_window);
    mvwprintw(status_window, 0, 1, "%s", status.c_str());
    wrefresh(status_window);
}

void UIManager::drawMessage(const std::string& sender, const std::string& message, Color color) {
    std::string full_message = sender + ": " + message;

    wattron(message_window, COLOR_PAIR(static_cast<int>(color)));
    wprintw(message_window, "%s\n", full_message.c_str());
    wattroff(message_window, COLOR_PAIR(static_cast<int>(color)));
    
    wrefresh(message_window);
    wmove(input_window, 0, getcurx(input_window));
    wrefresh(input_window);
}

std::string UIManager::getUserInput() {
    char buffer[256];
    wmove(input_window, 0, 0);
    wclear(input_window);
    wgetnstr(input_window, buffer, 255);
    return std::string(buffer);
}

void UIManager::clearInput() {
    wclear(input_window);
    wrefresh(input_window);
}

void UIManager::refreshAll() {
    refresh();
    wrefresh(status_window);
    wrefresh(message_window);
    wrefresh(input_window);
}

void UIManager::debugLog(const std::string &log)
{
    drawMessage("debug", log, Color::Red);

    // Alguns erros fazem a ui fechar ai n da pra er os logs.
    // use essa função pra escrever os logs em um arquivo
    //writeDebugToFile(log);
}

void UIManager::writeDebugToFile(const std::string &log) {
    std::string filename = "debugLog.txt";
    if (std::filesystem::exists(filename)) {
        int index = 1;
        while (true) {
            std::ostringstream ss;
            ss << "debugLog" << index << ".txt";
            if (!std::filesystem::exists(ss.str())) {
                filename = ss.str();
                break;
            }
            ++index;
        }
    }

    // Append the log to the file
    std::ofstream outFile(filename, std::ios::app);
    if (outFile.is_open()) {
        outFile << log << std::endl;
    }
}