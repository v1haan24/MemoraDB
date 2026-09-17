#include "lexer.h"
#include <cctype>
#include <stdexcept>
#include <unordered_map>

// ================================================================
// Constructor
// ================================================================

Lexer::Lexer(const std::string& source)
    : input(source), position(0), line(1), column(1) {}

// ================================================================
// Character helpers
// ================================================================

char Lexer::currentChar() const {
    return isAtEnd() ? '\0' : input[position];
}

char Lexer::peekChar(size_t offset) const {
    size_t index = position + offset;
    return index >= input.size() ? '\0' : input[index];
}

bool Lexer::isAtEnd() const {
    return position >= input.size();
}

void Lexer::advance() {
    if (isAtEnd()) return;
    if (input[position] == '\n') { ++line; column = 1; }
    else { ++column; }
    ++position;
}

bool Lexer::isWhitespace(char c) const {
    return std::isspace(static_cast<unsigned char>(c));
}

bool Lexer::isIdentifierStart(char c) const {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool Lexer::isIdentifierPart(char c) const {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

// ================================================================
// Token creation
// ================================================================

Token Lexer::makeToken(TokenType type, const std::string& value,
                        int startLine, int startColumn) {
    return Token{type, value, startLine, startColumn};
}

// ================================================================
// Keyword lookup
// ================================================================

TokenType Lexer::keywordType(const std::string& word) const {
    static const std::unordered_map<std::string, TokenType> keywords = {
        // DDL
        {"CREATE", TokenType::CREATE}, {"DROP", TokenType::DROP}, {"TABLE", TokenType::TABLE},
        {"INSERT", TokenType::INSERT}, {"INTO", TokenType::INTO}, {"VALUES", TokenType::VALUES},
        {"UPDATE", TokenType::UPDATE}, {"SET", TokenType::SET},
        {"DELETE", TokenType::DELETE},
        {"SELECT", TokenType::SELECT}, {"FROM", TokenType::FROM},
        // Filtering / sorting
        {"WHERE", TokenType::WHERE}, {"ORDER", TokenType::ORDER}, {"BY", TokenType::BY},
        {"LIMIT", TokenType::LIMIT}, {"ASC", TokenType::ASC}, {"DESC", TokenType::DESC},
        {"AND", TokenType::AND},   // needed by: BETWEEN <date> AND <date>
        // Schema
        {"PRIMARY", TokenType::PRIMARY}, {"KEY", TokenType::KEY},
        {"INT", TokenType::INT}, {"FLOAT", TokenType::FLOAT},
        {"STRING", TokenType::STRING}, {"BOOL", TokenType::BOOL},
        {"SEMANTIC", TokenType::SEMANTIC},
        // Temporal
        {"AS", TokenType::AS}, {"OF", TokenType::OF},
        {"BETWEEN", TokenType::BETWEEN}, {"SNAPSHOT", TokenType::SNAPSHOT},
        {"COMPARE", TokenType::COMPARE}, {"EVOLUTION", TokenType::EVOLUTION}, {"HISTORY", TokenType::HISTORY},
        {"ROLLBACK", TokenType::ROLLBACK}, {"TO", TokenType::TO}, {"COMPACT", TokenType::COMPACT},
        // Semantic
        {"SIMILAR", TokenType::SIMILAR},
    };

    auto it = keywords.find(word);
    return it != keywords.end() ? it->second : TokenType::IDENTIFIER;
}

// ================================================================
// Identifier / keyword scanner
// ================================================================

Token Lexer::scanIdentifierOrKeyword() {
    int startLine = line, startColumn = column;
    std::string value;

    while (!isAtEnd() && isIdentifierPart(currentChar())) {
        value += currentChar();
        advance();
    }

    std::string upper = value;
    for (char& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    return makeToken(keywordType(upper), value, startLine, startColumn);
}

// ================================================================
// Number scanner
// ================================================================

Token Lexer::scanNumber() {
    int startLine = line, startColumn = column;
    std::string value;

    while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(currentChar()))) {
        value += currentChar();
        advance();
    }

    bool hasDecimal = false;
    if (currentChar() == '.' && std::isdigit(static_cast<unsigned char>(peekChar()))) {
        hasDecimal = true;
        value += currentChar();
        advance();
        while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(currentChar()))) {
            value += currentChar();
            advance();
        }
    }

    return makeToken(hasDecimal ? TokenType::FLOAT_LITERAL : TokenType::INTEGER_LITERAL,
                      value, startLine, startColumn);
}

// ================================================================
// String scanner
//
// Supports \n \t \\ \" \' ; an unrecognized escape keeps the escaped
// character as-is.
// ================================================================

Token Lexer::scanString() {
    int startLine = line, startColumn = column;
    char quote = currentChar();
    advance(); // opening quote

    std::string value;
    while (!isAtEnd()) {
        if (currentChar() == quote) {
            advance(); // closing quote
            return makeToken(TokenType::STRING_LITERAL, value, startLine, startColumn);
        }

        if (currentChar() == '\\') {
            advance();
            if (isAtEnd()) break;
            switch (currentChar()) {
                case 'n':  value += '\n'; break;
                case 't':  value += '\t'; break;
                case '\\': value += '\\'; break;
                case '"':  value += '"';  break;
                case '\'': value += '\''; break;
                default:   value += currentChar(); break; // unknown escape: keep as-is
            }
            advance();
        } else {
            value += currentChar();
            advance();
        }
    }

    throw std::runtime_error("Unterminated string literal at line " +
                              std::to_string(startLine) + ", column " + std::to_string(startColumn));
}

// ================================================================
// Operator scanner: =  !=  <  <=  >  >=
// ================================================================

Token Lexer::scanOperator() {
    int startLine = line, startColumn = column;
    char c = currentChar();
    advance();

    // If the next char is '=', consume it and return `withEq`;
    // otherwise return `alone` (unconsumed).
    auto maybeEq = [&](TokenType withEq, TokenType alone) {
        std::string text(1, c);
        if (currentChar() == '=') { text += '='; advance(); return makeToken(withEq, text, startLine, startColumn); }
        return makeToken(alone, text, startLine, startColumn);
    };

    switch (c) {
        case '=': return makeToken(TokenType::EQUAL, "=", startLine, startColumn);
        case '!': return maybeEq(TokenType::NOT_EQUAL, TokenType::UNKNOWN); // bare '!' is invalid
        case '<': return maybeEq(TokenType::LESS_EQUAL, TokenType::LESS);
        case '>': return maybeEq(TokenType::GREATER_EQUAL, TokenType::GREATER);
    }
    return makeToken(TokenType::UNKNOWN, std::string(1, c), startLine, startColumn); // unreachable via nextToken()'s dispatch
}

// ================================================================
// Symbol scanner: *  (  )  ,  ; -
// ================================================================

Token Lexer::scanSymbol() {
    static const std::unordered_map<char, TokenType> symbols = {
        {'*', TokenType::STAR}, {'(', TokenType::LPAREN}, {')', TokenType::RPAREN},
        {',', TokenType::COMMA}, {';', TokenType::SEMICOLON}, {'-', TokenType::MINUS},
    };

    int startLine = line, startColumn = column;
    char c = currentChar();
    advance();

    auto it = symbols.find(c);
    TokenType type = it != symbols.end() ? it->second : TokenType::UNKNOWN;
    return makeToken(type, std::string(1, c), startLine, startColumn);
}

// ================================================================
// Get next token
// ================================================================

Token Lexer::nextToken() {
    while (!isAtEnd() && isWhitespace(currentChar())) advance();

    if (isAtEnd()) return makeToken(TokenType::END_OF_FILE, "", line, column);

    char c = currentChar();

    if (isIdentifierStart(c)) return scanIdentifierOrKeyword();
    if (std::isdigit(static_cast<unsigned char>(c))) return scanNumber();
    if (c == '"' || c == '\'') return scanString();
    if (c == '=' || c == '!' || c == '<' || c == '>') return scanOperator();
    if (c == '*' || c == '(' || c == ')' || c == ',' || c == ';' || c == '-') return scanSymbol();

    int startLine = line, startColumn = column;
    advance();
    return makeToken(TokenType::UNKNOWN, std::string(1, c), startLine, startColumn);
}

// ================================================================
// Tokenize entire input
// ================================================================

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    Token token;
    do {
        token = nextToken();
        tokens.push_back(token);
    } while (token.type != TokenType::END_OF_FILE);
    return tokens;
}

// ================================================================
// Token type -> string
// ================================================================

std::string tokenTypeToString(TokenType type) {
    static const std::unordered_map<TokenType, std::string> names = {
        {TokenType::END_OF_FILE, "END_OF_FILE"}, {TokenType::UNKNOWN, "UNKNOWN"},

        {TokenType::CREATE, "CREATE"}, {TokenType::DROP, "DROP"}, {TokenType::TABLE, "TABLE"},
        {TokenType::INSERT, "INSERT"}, {TokenType::INTO, "INTO"}, {TokenType::VALUES, "VALUES"},
        {TokenType::UPDATE, "UPDATE"}, {TokenType::SET, "SET"},
        {TokenType::DELETE, "DELETE"},
        {TokenType::SELECT, "SELECT"}, {TokenType::FROM, "FROM"},

        {TokenType::WHERE, "WHERE"}, {TokenType::ORDER, "ORDER"}, {TokenType::BY, "BY"},
        {TokenType::LIMIT, "LIMIT"}, {TokenType::ASC, "ASC"}, {TokenType::DESC, "DESC"},
        {TokenType::AND, "AND"},

        {TokenType::PRIMARY, "PRIMARY"}, {TokenType::KEY, "KEY"},
        {TokenType::INT, "INT"}, {TokenType::FLOAT, "FLOAT"},
        {TokenType::STRING, "STRING"}, {TokenType::BOOL, "BOOL"},
        {TokenType::SEMANTIC, "SEMANTIC"},

        {TokenType::AS, "AS"}, {TokenType::OF, "OF"},
        {TokenType::BETWEEN, "BETWEEN"}, {TokenType::SNAPSHOT, "SNAPSHOT"},
        {TokenType::COMPARE, "COMPARE"}, {TokenType::EVOLUTION, "EVOLUTION"}, {TokenType::HISTORY, "HISTORY"},
        {TokenType::ROLLBACK, "ROLLBACK"}, {TokenType::TO, "TO"}, {TokenType::COMPACT, "COMPACT"},
        {TokenType::SIMILAR, "SIMILAR"},

        {TokenType::IDENTIFIER, "IDENTIFIER"},
        {TokenType::INTEGER_LITERAL, "INTEGER_LITERAL"},
        {TokenType::FLOAT_LITERAL, "FLOAT_LITERAL"},
        {TokenType::STRING_LITERAL, "STRING_LITERAL"},

        {TokenType::EQUAL, "EQUAL"}, {TokenType::NOT_EQUAL, "NOT_EQUAL"},
        {TokenType::LESS, "LESS"}, {TokenType::LESS_EQUAL, "LESS_EQUAL"},
        {TokenType::GREATER, "GREATER"}, {TokenType::GREATER_EQUAL, "GREATER_EQUAL"},

        {TokenType::STAR, "STAR"}, {TokenType::LPAREN, "LPAREN"}, {TokenType::RPAREN, "RPAREN"},
        {TokenType::COMMA, "COMMA"}, {TokenType::SEMICOLON, "SEMICOLON"}, {TokenType::MINUS, "MINUS"},
    };

    auto it = names.find(type);
    return it != names.end() ? it->second : "UNKNOWN";
}