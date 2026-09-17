#include "parser.h"
CompareStmt Parser::parseCompare() {
    expect(TokenType::COMPARE, "");
    const Token& nameTok = expect(TokenType::IDENTIFIER, "as the table name after COMPARE");
    CompareStmt stmt;
    stmt.tableName = nameTok.value;
    
    expect(TokenType::WHERE, "after table name in COMPARE (WHERE is required here)");
    stmt.where = parseCondition();

    expect(TokenType::BETWEEN, "after the WHERE condition in COMPARE");
    stmt.rangeStart = parseDateLiteral();
    expect(TokenType::AND, "between the two dates in COMPARE ... BETWEEN ... AND ...");
    stmt.rangeEnd = parseDateLiteral();
    return stmt;
}
EvolutionStmt Parser::parseEvolution() {
    expect(TokenType::EVOLUTION, "");
    const Token& nameTok = expect(TokenType::IDENTIFIER, "as the table name after EVOLUTION");
    EvolutionStmt stmt;
    stmt.tableName = nameTok.value;
    expect(TokenType::WHERE, "after table name in EVOLUTION (WHERE is required here)");
    stmt.where = parseCondition();

    expect(TokenType::BETWEEN, "after the WHERE condition in EVOLUTION");
    stmt.rangeStart = parseDateLiteral();
    expect(TokenType::AND, "between the two dates in EVOLUTION ... BETWEEN ... AND ...");
    stmt.rangeEnd = parseDateLiteral();

    return stmt;
}
HistoryStmt Parser::parseHistory() {
    expect(TokenType::HISTORY, "");
    const Token& nameTok = expect(TokenType::IDENTIFIER, "as the table name after HISTORY");

    HistoryStmt stmt;
    stmt.tableName = nameTok.value;
    expect(TokenType::WHERE, "after table name in HISTORY (WHERE is required here)");
    stmt.where = parseCondition();
    return stmt;
}