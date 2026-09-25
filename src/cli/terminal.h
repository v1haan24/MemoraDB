#pragma once
#include <initializer_list>
#include <string>
#include <vector>

namespace term {

enum Color {
    RESET,
    BOLD,
    DIM,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN,
    GRAY,
    WHITE,
    LIGHT_YELLOW
};

struct Rgb {
    int r;
    int g;
    int b;
};

void init();
bool colorsEnabled();
bool supportsTrueColor();
int width();
void clear();
void setTitle(const std::string& title);

std::string paint(const std::string& text, Color color);
std::string paint(const std::string& text, std::initializer_list<Color> styles);
std::string rgbEscape(const Rgb& c, bool bold);
std::string rgbPaint(const std::string& text, const Rgb& c, bool bold = false);
Rgb bannerGradient(double t);
std::string gradientText(const std::string& text);
std::string gradientLine(const std::string& line, int row, int rows, int totalWidth);
std::string gradientRule(const std::string& glyph, int width);
std::string hr(char ch = '-');

void printTable(const std::vector<std::string>& headers,
                const std::vector<std::vector<std::string>>& rows);

} // namespace term
