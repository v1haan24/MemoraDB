#include "parser.h"
#include <algorithm>

Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {
    if (this->tokens.empty()) {
        throw std::invalid_argument(
            "Parser: token stream must not be empty "
            "(Lexer::tokenize() should always produce at least END_OF_FILE)");
    }
}
const Token& Parser::peek(size_t offset) const {
    size_t index = current + offset;
    if (index >= tokens.size()) return tokens.back();
    return tokens[index];
}
const Token& Parser::advance() {
    const Token& current_token = peek();
    if (!isAtEnd()) ++current;
    return current_token;
}
bool Parser::check(TokenType type) const {
    return peek().type == type;
}
bool Parser::match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}
const Token& Parser::expect(TokenType type, const std::string& context) {
    if (check(type)) return advance();
    const Token& actual = peek();
    throw ParseError(describeMismatch(type, actual, context), actual.line, actual.column);
}
bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}
size_t Parser::position() const {
    return current;
}
void Parser::seek(size_t pos) {
    current = std::min(pos, tokens.size() - 1);
}
std::string Parser::describeMismatch(TokenType expected, const Token& actual,
                                      const std::string& context) const {
    std::string message = "Expected " + tokenTypeToString(expected) +
                           " but got " + tokenTypeToString(actual.type);
    if (!actual.value.empty()) message += " ('" + actual.value + "')";
    if (!context.empty()) message += " " + context;
    message += " at line " + std::to_string(actual.line) +
                ", column " + std::to_string(actual.column);
    return message;
}