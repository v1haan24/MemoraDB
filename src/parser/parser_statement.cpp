#include "parser.h"
Statement Parser::parseStatement() {
    switch (peek().type) {
        case TokenType::CREATE:    return parseCreateTable();
        case TokenType::DROP:      return parseDropTable();
        case TokenType::DESCRIBE:  return parseDescribeTable();
        case TokenType::SHOW:      return parseShowTables();
        case TokenType::INSERT:    return parseInsert();
        case TokenType::UPDATE:    return parseUpdate();
        case TokenType::DELETE:    return parseDelete();
        case TokenType::SELECT:    return parseSelect();
        case TokenType::COMPARE:   return parseCompare();
        case TokenType::EVOLUTION: return parseEvolution();
        case TokenType::HISTORY:   return parseHistory();
        case TokenType::ROLLBACK:  return parseRollback();
        case TokenType::COMPACT:   return parseCompact();
        default: {
            const Token& actual = peek();
            throw ParseError(
                "Expected the start of a statement (CREATE, DROP, DESCRIBE, SHOW, INSERT, UPDATE, DELETE, "
                "SELECT, COMPARE, EVOLUTION, HISTORY, ROLLBACK, or COMPACT) but got " +
                    tokenTypeToString(actual.type) +
                    (actual.value.empty() ? "" : " ('" + actual.value + "')"),
                actual.line, actual.column);
        }
    }
}
std::vector<Statement> Parser::parseProgram() {
    std::vector<Statement> statements;
    while (!isAtEnd()) {
        Statement stmt = parseStatement();
        expect(TokenType::SEMICOLON, "at the end of the statement");
        statements.push_back(std::move(stmt));
    }
    return statements;
}