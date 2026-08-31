#include <iostream>
#include "../src/lexer/lexer.h"

int main() {

    std::string query = "SELECT * FROM students WHERE cgpa >= 8.0 ORDER BY cgpa DESC LIMIT 5;";
    /*
    R"(CREATE TABLE students (
        id INT PRIMARY KEY,
        name STRING(50),
        content STRING(40) SEMANTIC,
        age INT,
        cgpa FLOAT
    );)";
    */
    //"COMPACT students TO '2026-08-21';";
    //"SELECT * FROM students WHERE id = -8;";


    Lexer lexer(query);

    std::vector<Token> tokens = lexer.tokenize();


    for (const Token& token : tokens) {

        std::cout
            << token.line
            << ":"
            << token.column
            << "  "
            << tokenTypeToString(token.type)
            << "  ["
            << token.value
            << "]\n";
    }


    return 0;
}