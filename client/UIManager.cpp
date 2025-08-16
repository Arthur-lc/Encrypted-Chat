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

    start_color(); // Inicia as cores
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);

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

    int current_line = 0;
    for (size_t i = 0; i < messages.size() && i < max_messages; ++i) {
        std::string msg_to_display = messages[start_index + i];
        bool isDebug = (msg_to_display.rfind("debug:", 0) == 0);
        size_t pair = 1;
        if (isDebug) {
            pair = 2;
        }

        wattron(message_window, COLOR_PAIR(pair));
        
        // Lidar com mensagens longas que quebram em múltiplas linhas
        int max_width = getmaxx(message_window) - 2; // -2 para margem
        if (msg_to_display.length() <= max_width) {
            // Mensagem cabe em uma linha
            mvwprintw(message_window, current_line, 1, "%s", msg_to_display.c_str());
            current_line++;
        } else {
            // Mensagem quebra em múltiplas linhas
            size_t pos = 0;
            while (pos < msg_to_display.length() && current_line < max_messages) {
                std::string line = msg_to_display.substr(pos, max_width);
                mvwprintw(message_window, current_line, 1, "%s", line.c_str());
                current_line++;
                pos += max_width;
            }
        }
        
        wattroff(message_window, COLOR_PAIR(pair));
    }
    
    wrefresh(message_window);
    
    // Restaurar o cursor na janela de input
    wmove(input_window, 0, 0);
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
    drawMessage("debug", log);
}