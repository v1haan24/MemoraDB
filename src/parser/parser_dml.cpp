#include "parser.h"

InsertStmt Parser::parseInsert() {
    expect(TokenType::INSERT, "");
    expect(TokenType::INTO, "after INSERT");
    const Token& nameTok = expect(TokenType::IDENTIFIER, "as the table name after INSERT INTO");
    InsertStmt stmt;
    stmt.tableName = nameTok.value;

    expect(TokenType::VALUES, "after table name in INSERT");
    expect(TokenType::LPAREN, "to begin the value list");
    stmt.values.push_back(parseValue());
    while (match(TokenType::COMMA)) {
        stmt.values.push_back(parseValue());
    }
    expect(TokenType::RPAREN, "to close the value list");
    return stmt;
}
UpdateStmt Parser::parseUpdate() {
    expect(TokenType::UPDATE, "");
    const Token& nameTok = expect(TokenType::IDENTIFIER, "as the table name after UPDATE");
    UpdateStmt stmt;
    stmt.tableName = nameTok.value;

    expect(TokenType::SET, "after table name in UPDATE");

    auto parseAssignment = [this]() -> Assignment {
        const Token& colTok = expect(TokenType::IDENTIFIER, "as a column name in SET");
        expect(TokenType::EQUAL, "after column name in SET assignment");
        Value v = parseValue();
        return Assignment{colTok.value, v};
    };
    stmt.assignments.push_back(parseAssignment());
    while (match(TokenType::COMMA)) {
        stmt.assignments.push_back(parseAssignment());
    }
    if (match(TokenType::WHERE)) {
        stmt.where = parseCondition();
    }
    return stmt;
}
DeleteStmt Parser::parseDelete() {
    expect(TokenType::DELETE, "");
    expect(TokenType::FROM, "after DELETE");
    const Token& nameTok = expect(TokenType::IDENTIFIER, "as the table name after DELETE FROM");
    DeleteStmt stmt;
    stmt.tableName = nameTok.value;
    if (match(TokenType::WHERE)) {
        stmt.where = parseCondition();
    }
    return stmt;
}