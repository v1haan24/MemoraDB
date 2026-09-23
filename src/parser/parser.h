#pragma once
#include "../lexer/token.h"
#include "ast.h"
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdint>

struct ParseError : std::runtime_error {
    int32_t line;
    int32_t column;
    ParseError(const std::string& message, int32_t line, int32_t column)
        : std::runtime_error(message), line(line), column(column) {}
};
class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    const Token& peek(size_t offset = 0) const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    const Token& expect(TokenType type, const std::string& context);
    bool isAtEnd() const;
    size_t position() const;
    void seek(size_t pos);

    std::vector<Statement> parseProgram();
   
    Statement parseStatement();

    Value parseValue();
    DateLiteral parseDateLiteral();
    CompareOp parseCompareOp();
    Condition parseCondition();

    CreateTableStmt parseCreateTable();
    ColumnDef parseColumnDef();
    DropTableStmt parseDropTable();
    DescribeTableStmt parseDescribeTable();
    ShowTablesStmt parseShowTables();

    InsertStmt parseInsert();
    UpdateStmt parseUpdate();
    DeleteStmt parseDelete();
    SelectStmt parseSelect();

    CompareStmt parseCompare();
    EvolutionStmt parseEvolution();
    HistoryStmt parseHistory();

    RollbackStmt parseRollback();
    CompactStmt parseCompact();

private:
    std::vector<Token> tokens;
    size_t current = 0;
    std::string describeMismatch(TokenType expected, const Token& actual,
                                  const std::string& context) const;
    void parseSelectList(SelectStmt& stmt);
    void parseTemporalClause(SelectStmt& stmt);
};