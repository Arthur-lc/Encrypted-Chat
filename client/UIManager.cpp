#include "UIManager.h"
#include <stdexcept>

UIManager::UIManager() {
    initscr();
    if (stdscr == NULL) {
        throw std::runtime_error("Error initializing ncurses.");
    }

    // Basic settings
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(1);

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
    keypad(input_window, TRUE);

    max_messages = height - 3;

    // Initial draw
    drawBorders();
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

void UIManager::drawBorders() {
    mvwhline(status_window, 0, 0, ACS_HLINE, getmaxx(stdscr));
    wrefresh(status_window);
}

void UIManager::updateStatus(const std::string& status) {
    wclear(status_window);
    mvwprintw(status_window, 0, 1, "%s", status.c_str());
    wrefresh(status_window);
}

void UIManager::drawMessage(const std::string& sender, const std::string& message) {
    // Add message to history
    std::string full_message = sender + ": " + message;
    messages.push_back(full_message);

    // Redraw all messages
    wclear(message_window);
    int start_index = 0;
    if (messages.size() > max_messages) {
        start_index = messages.size() - max_messages;
    }

    for (size_t i = 0; i < messages.size() && i < max_messages; ++i) {
        mvwprintw(message_window, i, 1, "%s", messages[start_index + i].c_str());
    }
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
