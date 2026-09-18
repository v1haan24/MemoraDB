#pragma once
#include <initializer_list>
#include <string>
#include <vector>

namespace term {
enum Color { RESET, BOLD, DIM, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, GRAY, WHITE };
void init();
bool colorsEnabled();
int width();
void clear();
void setTitle(const std::string& title);
std::string paint(const std::string& text, Color color);
std::string paint(const std::string& text, std::initializer_list<Color> styles);
std::string hr(char ch = '-');
void printTable(const std::vector<std::string>& headers,
                const std::vector<std::vector<std::string>>& rows);
}
