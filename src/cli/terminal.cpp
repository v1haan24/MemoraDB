#include "terminal.h"
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
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

bool tty() {
    return TERM_ISATTY(TERM_FILENO(stdout)) != 0;
}

const char* code(term::Color c) {
    switch (c) {
        case term::RESET:        return "\033[0m";
        case term::BOLD:         return "\033[1m";
        case term::DIM:          return "\033[2m";
        case term::RED:          return "\033[31m";
        case term::GREEN:        return "\033[32m";
        case term::YELLOW:       return "\033[33m";
        case term::BLUE:         return "\033[34m";
        case term::MAGENTA:      return "\033[35m";
        case term::CYAN:         return "\033[36m";
        case term::GRAY:         return "\033[90m";
        case term::WHITE:        return "\033[37m";
        case term::LIGHT_YELLOW: return "\033[38;5;229m";
    }
    return "";
}

term::Rgb mixRgb(const term::Rgb& a, const term::Rgb& b, double t) {
    return term::Rgb{
        static_cast<int>(a.r + (b.r - a.r) * t),
        static_cast<int>(a.g + (b.g - a.g) * t),
        static_cast<int>(a.b + (b.b - a.b) * t)
    };
}

int toAnsi256(const term::Rgb& c) {
    auto level = [](int v) {
        return static_cast<int>(v / 255.0 * 5.0 + 0.5);
    };
    return 16 + 36 * level(c.r) + 6 * level(c.g) + level(c.b);
}

} // namespace

void term::init() {
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (out != INVALID_HANDLE_VALUE && GetConsoleMode(out, &mode)) {
        SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
    SetConsoleOutputCP(CP_UTF8);
#endif
}

bool term::colorsEnabled() {
    static const bool enabled = [] {
        if (std::getenv("NO_COLOR") || std::getenv("MEMORA_NO_COLOR")) return false;
#ifndef _WIN32
        if (const char* t = std::getenv("TERM")) {
            if (std::string(t) == "dumb") return false;
        }
#endif
        return tty();
    }();
    return enabled;
}

bool term::supportsTrueColor() {
    static const bool enabled = [] {
#ifdef _WIN32
        return true;
#else
        const char* ct = std::getenv("COLORTERM");
        if (ct) {
            const std::string v(ct);
            if (v == "truecolor" || v == "24bit") return true;
        }
        return false;
#endif
    }();
    return enabled;
}

int term::width() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(out, &info)) {
        return static_cast<int>(info.srWindow.Right - info.srWindow.Left + 1);
    }
#else
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col) return ws.ws_col;
#endif
    return 80;
}

void term::clear() {
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info{};
    DWORD written = 0;

    if (out != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(out, &info)) {
        const DWORD cells = static_cast<DWORD>(info.dwSize.X) * static_cast<DWORD>(info.dwSize.Y);
        const COORD home{0, 0};
        FillConsoleOutputCharacterA(out, ' ', cells, home, &written);
        FillConsoleOutputAttribute(out, info.wAttributes, cells, home, &written);
        SetConsoleCursorPosition(out, home);
    }

    if (colorsEnabled()) {
        std::cout << "\033[3J\033[2J\033[H";
    }
#else
    if (colorsEnabled()) {
        std::cout << "\033[3J\033[2J\033[H";
    } else {
        std::system("clear");
    }
#endif
    std::cout.flush();
}

void term::setTitle(const std::string& title) {
#ifdef _WIN32
    SetConsoleTitleA(title.c_str());
#else
    if (colorsEnabled()) {
        std::cout << "\033]0;" << title << "\007" << std::flush;
    }
#endif
}

std::string term::paint(const std::string& text, Color c) {
    return colorsEnabled() ? std::string(code(c)) + text + code(RESET) : text;
}

std::string term::paint(const std::string& text, std::initializer_list<Color> cs) {
    if (!colorsEnabled()) return text;
    std::string p;
    for (Color c : cs) p += code(c);
    return p + text + code(RESET);
}

std::string term::rgbEscape(const Rgb& c, bool bold) {
    const std::string prefix = std::string("\033[") + (bold ? "1;" : "");
    if (supportsTrueColor()) {
        return prefix + "38;2;" + std::to_string(c.r) + ";"
               + std::to_string(c.g) + ";" + std::to_string(c.b) + "m";
    }
    return prefix + "38;5;" + std::to_string(toAnsi256(c)) + "m";
}

std::string term::rgbPaint(const std::string& text, const Rgb& c, bool bold) {
    if (!colorsEnabled()) return text;
    return rgbEscape(c, bold) + text + "\033[0m";
}

term::Rgb term::bannerGradient(double t) {
    static const Rgb stops[4] = {
        {0, 229, 255},
        {99, 102, 241},
        {192, 72, 255},
        {255, 84, 160}
    };

    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;

    const double scaled = t * 3.0;
    int i = static_cast<int>(scaled);
    if (i > 2) i = 2;

    return mixRgb(stops[i], stops[i + 1], scaled - i);
}

std::string term::gradientLine(const std::string& line, int row, int rows, int totalWidth) {
    if (!colorsEnabled()) return line;

    const double span = static_cast<double>(totalWidth + rows * 3);
    std::string out;

    for (size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (ch == ' ') {
            out += ch;
            continue;
        }
        const double t = (static_cast<double>(i) + row * 3.0) / span;
        out += rgbEscape(bannerGradient(t), true);
        out += ch;
    }
    return out + "\033[0m";
}

std::string term::gradientText(const std::string& text) {
    if (!colorsEnabled()) return text;

    std::string out;
    const double span = text.size() > 1 ? static_cast<double>(text.size() - 1) : 1.0;

    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == ' ') {
            out += ' ';
            continue;
        }
        out += rgbEscape(bannerGradient(i / span), true);
        out += text[i];
    }
    return out + "\033[0m";
}

std::string term::gradientRule(const std::string& glyph, int width) {
    if (width < 1) return "";

    std::string out;
    for (int i = 0; i < width; ++i) {
        if (colorsEnabled()) {
            const double t = width > 1 ? static_cast<double>(i) / (width - 1) : 0.0;
            out += rgbEscape(bannerGradient(t), true);
        }
        out += glyph;
    }
    if (colorsEnabled()) out += "\033[0m";
    return out;
}

std::string term::hr(char c) {
    return std::string(static_cast<size_t>(width()), c);
}

void term::printTable(const std::vector<std::string>& headers,
                      const std::vector<std::vector<std::string>>& rows) {
    if (headers.empty()) return;

    std::vector<size_t> widths(headers.size());
    for (size_t i = 0; i < headers.size(); ++i) {
        widths[i] = headers[i].size();
    }
    for (const auto& row : rows) {
        for (size_t i = 0; i < widths.size() && i < row.size(); ++i) {
            widths[i] = (std::max)(widths[i], row[i].size());
        }
    }

    auto printBorder = [&] {
        std::cout << '+';
        for (auto w : widths) {
            std::cout << std::string(w + 2, '-') << '+';
        }
        std::cout << '\n';
    };

    auto printRow = [&](const auto& r, bool isHeader = false) {
        std::cout << '|';
        for (size_t i = 0; i < widths.size(); ++i) {
            const std::string val = i < r.size() ? r[i] : "";
            const std::string out = isHeader ? paint(val, {BOLD, CYAN}) : val;
            std::cout << ' ' << out << std::string(widths[i] - val.size(), ' ') << " |";
        }
        std::cout << '\n';
    };

    printBorder();
    printRow(headers, true);
    printBorder();
    for (const auto& r : rows) {
        printRow(r);
    }
    printBorder();
}
