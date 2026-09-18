#include "terminal.h"
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define TERM_ISATTY _isatty
#define TERM_FILENO _fileno
#else
#include <sys/ioctl.h>
#include <unistd.h>
#define TERM_ISATTY isatty
#define TERM_FILENO fileno
#endif

namespace {
bool tty() { return TERM_ISATTY(TERM_FILENO(stdout)) != 0; }
const char* code(term::Color c) {
    switch (c) {
        case term::RESET: return "\033[0m"; case term::BOLD: return "\033[1m";
        case term::DIM: return "\033[2m"; case term::RED: return "\033[31m";
        case term::GREEN: return "\033[32m"; case term::YELLOW: return "\033[33m";
        case term::BLUE: return "\033[34m"; case term::MAGENTA: return "\033[35m";
        case term::CYAN: return "\033[36m"; case term::GRAY: return "\033[90m";
        case term::WHITE: return "\033[37m";
    }
    return "";
}
}
void term::init() {
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE); DWORD mode = 0;
    if (out != INVALID_HANDLE_VALUE && GetConsoleMode(out, &mode))
        SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleOutputCP(CP_UTF8);
#endif
}
bool term::colorsEnabled() {
    static const bool enabled = [] {
        if (std::getenv("NO_COLOR") || std::getenv("MEMORA_NO_COLOR")) return false;
#ifndef _WIN32
        if (const char* t = std::getenv("TERM")) if (std::string(t) == "dumb") return false;
#endif
        return tty();
    }();
    return enabled;
}
int term::width() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info; HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(out, &info))
        return static_cast<int>(info.srWindow.Right - info.srWindow.Left + 1);
#else
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col) return ws.ws_col;
#endif
    return 80;
}
void term::clear() {
    if (colorsEnabled()) std::cout << "\033[2J\033[H";
#ifdef _WIN32
    else std::system("cls");
#else
    else std::system("clear");
#endif
    std::cout.flush();
}
void term::setTitle(const std::string& title) {
#ifdef _WIN32
    SetConsoleTitleA(title.c_str());
#else
    if (colorsEnabled()) std::cout << "\033]0;" << title << "\007" << std::flush;
#endif
}
std::string term::paint(const std::string& text, Color c) {
    return colorsEnabled() ? std::string(code(c)) + text + code(RESET) : text;
}
std::string term::paint(const std::string& text, std::initializer_list<Color> cs) {
    if (!colorsEnabled()) return text;
    std::string p; for (Color c : cs) p += code(c);
    return p + text + code(RESET);
}
std::string term::hr(char c) { return std::string(static_cast<size_t>(width()), c); }
void term::printTable(const std::vector<std::string>& h,
                      const std::vector<std::vector<std::string>>& rows) {
    if (h.empty()) return;
    std::vector<size_t> w(h.size());
    for (size_t i = 0; i < h.size(); ++i) w[i] = h[i].size();
    for (const auto& r : rows) for (size_t i = 0; i < w.size() && i < r.size(); ++i)
        w[i] = std::max(w[i], r[i].size());
    auto border = [&] { std::cout << '+'; for (auto n : w) std::cout << std::string(n + 2, '-') << '+'; std::cout << '\n'; };
    auto row = [&](const auto& r) { std::cout << '|'; for (size_t i = 0; i < w.size(); ++i) {
        const std::string v = i < r.size() ? r[i] : "";
        std::cout << ' ' << std::left << std::setw((int)w[i]) << v << " |";
    } std::cout << '\n'; };
    border(); row(h); border(); for (const auto& r : rows) row(r); border();
}
