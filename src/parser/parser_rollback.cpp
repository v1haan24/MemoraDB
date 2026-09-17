#include "parser.h"

RollbackStmt Parser::parseRollback() {
    expect(TokenType::ROLLBACK, "");
    RollbackStmt stmt;
    if (match(TokenType::TABLE)) {
        stmt.wholeTable = true;
        const Token& nameTok = expect(TokenType::IDENTIFIER, "as the table name after ROLLBACK TABLE");
        stmt.tableName = nameTok.value;
        expect(TokenType::TO, "after table name in ROLLBACK TABLE");
        stmt.toDate = parseDateLiteral();
        return stmt;
    }
    stmt.wholeTable = false;
    const Token& nameTok = expect(TokenType::IDENTIFIER, "as the table name after ROLLBACK");
    stmt.tableName = nameTok.value;
    expect(TokenType::WHERE,
           "after table name in ROLLBACK (expected 'WHERE <condition> TO <date>', "
           "or 'ROLLBACK TABLE <name> TO <date>' for a whole-table rollback)");
    stmt.where = parseCondition();
    expect(TokenType::TO, "after the WHERE condition in ROLLBACK");
    stmt.toDate = parseDateLiteral();
    return stmt;
}
CompactStmt Parser::parseCompact() {
    expect(TokenType::COMPACT, "");
    expect(TokenType::TABLE, "after COMPACT");
    const Token& nameTok = expect(TokenType::IDENTIFIER, "as the table name after COMPACT TABLE");
    CompactStmt stmt;
    stmt.tableName = nameTok.value;

    expect(TokenType::TO, "after table name in COMPACT TABLE");
    stmt.toDate = parseDateLiteral();
    return stmt;
}