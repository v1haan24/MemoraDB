#include "parser.h"

void Parser::parseSelectList(SelectStmt& stmt) {
    if (match(TokenType::STAR)) {
        stmt.selectAll = true;
        return;
    }
    stmt.selectAll = false;
    const Token& first = expect(TokenType::IDENTIFIER, "as a column name in the select list");
    stmt.columns.push_back(first.value);
    while (match(TokenType::COMMA)) {
        const Token& col = expect(TokenType::IDENTIFIER, "as a column name in the select list");
        stmt.columns.push_back(col.value);
    }
}
void Parser::parseTemporalClause(SelectStmt& stmt) {
    if (match(TokenType::AS)) {
        expect(TokenType::OF, "after AS in a temporal clause (expected 'AS OF <date>')");
        stmt.temporalMode = TemporalMode::AS_OF;
        stmt.asOfDate = parseDateLiteral();
        return;
    }
    if (match(TokenType::BETWEEN)) {
        stmt.temporalMode = TemporalMode::BETWEEN;
        stmt.betweenStart = parseDateLiteral();
        expect(TokenType::AND, "between the two dates in BETWEEN ... AND ...");
        stmt.betweenEnd = parseDateLiteral();
        return;
    }
    if (match(TokenType::SNAPSHOT)) {
        stmt.temporalMode = TemporalMode::SNAPSHOT;
        stmt.snapshotDate = parseDateLiteral();
        return;
    }
}
SelectStmt Parser::parseSelect() {
    expect(TokenType::SELECT, "");
    SelectStmt stmt;
    parseSelectList(stmt);

    expect(TokenType::FROM, "after the select list in SELECT");
    const Token& nameTok = expect(TokenType::IDENTIFIER, "as the table name after FROM");
    stmt.tableName = nameTok.value;
    if (check(TokenType::AS) || check(TokenType::BETWEEN) || check(TokenType::SNAPSHOT)) {
        parseTemporalClause(stmt);
    }
    if (match(TokenType::WHERE)) {
        stmt.where = parseCondition();
    }
    if (match(TokenType::ORDER)) {
        expect(TokenType::BY, "after ORDER");
        const Token& colTok = expect(TokenType::IDENTIFIER, "as the column name in ORDER BY");
        stmt.orderByColumn = colTok.value;

        if (match(TokenType::DESC)) {
            stmt.orderDescending = true;
        } else {
            match(TokenType::ASC); // optional -- ASC is the default either way
            stmt.orderDescending = false;
        }
    }
    if (match(TokenType::LIMIT)) {
        const Token& limitTok = expect(TokenType::INTEGER_LITERAL, "after LIMIT");
        stmt.limit = std::stoi(limitTok.value);
    }
    return stmt;
}