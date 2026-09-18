#include "parser.h"

static bool isColumnTypeToken(TokenType t) {
    return t == TokenType::INT || t == TokenType::FLOAT ||
           t == TokenType::STRING || t == TokenType::BOOL;
}
ColumnDef Parser::parseColumnDef() {
    const Token& nameTok = expect(TokenType::IDENTIFIER, "as a column name");

    ColumnDef col;
    col.name = nameTok.value;
    const Token& typeTok = peek();
    if (!isColumnTypeToken(typeTok.type)) {
        throw ParseError(
            "Expected a column type (INT, FLOAT, STRING, or BOOL) but got " +
                tokenTypeToString(typeTok.type) + " for column '" + col.name + "'",
            typeTok.line, typeTok.column);
    }
    col.type = typeTok.type;
    advance();
    if (match(TokenType::LPAREN)) {
        if (col.type != TokenType::STRING) {
            throw ParseError(
                "Size suffix '(...)' is only valid on STRING columns, not on " +
                    tokenTypeToString(col.type) + " (column '" + col.name + "')",
                typeTok.line, typeTok.column);
        }
        const Token& sizeTok = expect(TokenType::INTEGER_LITERAL, "as the size in STRING(size)");
        col.size = std::stoi(sizeTok.value);
        expect(TokenType::RPAREN, "to close STRING(size)");
    }
    for (int i = 0; i < 2; ++i) {
        if (check(TokenType::PRIMARY)) {
            const Token& primaryTok = advance();
            expect(TokenType::KEY, "after PRIMARY");
            if (col.isPrimaryKey) {
                throw ParseError("Duplicate PRIMARY KEY on column '" + col.name + "'",
                                  primaryTok.line, primaryTok.column);
            }
            col.isPrimaryKey = true;
        } else if (check(TokenType::SEMANTIC)) {
            const Token& semanticTok = advance();
            if (col.isSemantic) {
                throw ParseError("Duplicate SEMANTIC on column '" + col.name + "'",
                                  semanticTok.line, semanticTok.column);
            }
            col.isSemantic = true;
        } else {
            break;
        }
    }

    return col;
}
CreateTableStmt Parser::parseCreateTable() {
    expect(TokenType::CREATE, "");
    expect(TokenType::TABLE, "after CREATE");
    const Token& nameTok = expect(TokenType::IDENTIFIER, "as the table name after CREATE TABLE");

    CreateTableStmt stmt;
    stmt.tableName = nameTok.value;
    expect(TokenType::LPAREN, "to begin the column list");
    stmt.columns.push_back(parseColumnDef());
    while (match(TokenType::COMMA)) {
        stmt.columns.push_back(parseColumnDef());
    }
    expect(TokenType::RPAREN, "to close the column list");
    return stmt;
}
DropTableStmt Parser::parseDropTable() {
    expect(TokenType::DROP, "");
    expect(TokenType::TABLE, "after DROP");
    const Token& nameTok = expect(TokenType::IDENTIFIER, "as the table name after DROP TABLE");

    DropTableStmt stmt;
    stmt.tableName = nameTok.value;
    return stmt;
}
DescribeTableStmt Parser::parseDescribeTable() {
    expect(TokenType::DESCRIBE, "");
    expect(TokenType::TABLE, "after DESCRIBE");
    const Token& nameTok = expect(TokenType::IDENTIFIER, "as the table name after DESCRIBE TABLE");

    DescribeTableStmt stmt;
    stmt.tableName = nameTok.value;
    return stmt;
}
ShowTablesStmt Parser::parseShowTables() {
    expect(TokenType::SHOW, "");
    expect(TokenType::TABLES, "after SHOW");

    return ShowTablesStmt{};
}