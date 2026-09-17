#pragma once

#include <string>
#include <vector>

#include "token.h"


class Lexer {

private:

    std::string input;

    size_t position;

    int line;
    int column;


    // ------------------------------------------------------------
    // Character helpers
    // ------------------------------------------------------------

    char currentChar() const;
    char peekChar(size_t offset = 1) const;
    bool isAtEnd() const;
    void advance();
    bool isWhitespace(char c) const;
    bool isIdentifierStart(char c) const;
    bool isIdentifierPart(char c) const;


    // ------------------------------------------------------------
    // Token creation
    // ------------------------------------------------------------

    Token makeToken(
        TokenType type,
        const std::string& value,
        int startLine,
        int startColumn
    );


    // ------------------------------------------------------------
    // Lexing functions
    // ------------------------------------------------------------

    Token scanIdentifierOrKeyword();
    Token scanNumber();
    Token scanString();
    Token scanOperator();
    Token scanSymbol();

    // ------------------------------------------------------------
    // Keyword lookup
    // ------------------------------------------------------------

    TokenType keywordType(const std::string& word) const;


public:
    explicit Lexer(const std::string& source);

    // Get the next token.
    Token nextToken();

    // Tokenize the entire input.
    std::vector<Token> tokenize();
};