#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "token.h"


class Lexer {

private:

    std::string input;

    size_t position;

    int32_t line;
    int32_t column;

    char currentChar() const;
    char peekChar(size_t offset = 1) const;
    bool isAtEnd() const;
    void advance();
    bool isWhitespace(char c) const;
    bool isIdentifierStart(char c) const;
    bool isIdentifierPart(char c) const;

    Token makeToken(
        TokenType type,
        const std::string& value,
        int32_t startLine,
        int32_t startColumn
    );

    Token scanIdentifierOrKeyword();
    Token scanNumber();
    Token scanString();
    Token scanOperator();
    Token scanSymbol();

    TokenType keywordType(const std::string& word) const;


public:
    explicit Lexer(const std::string& source);
    Token nextToken(); 
    std::vector<Token> tokenize();
};