#pragma once
#include "../lexer/token.h"
#include <string>
#include <vector>
#include <optional>
#include <variant>
struct Value {
    enum class Kind { INT, FLOAT, STRING } kind;
    std::string raw;
    long long intVal = 0;
    double floatVal = 0.0;
    std::string strVal;
};
struct DateLiteral {
    int year = 0;
    int month = 0;
    int day = 0;
};
enum class CompareOp { EQ, NE, LT, LE, GT, GE, SIMILAR_TO };
struct Condition {
    std::string column;
    CompareOp op;
    Value value;
};
struct ColumnDef {
    std::string name;
    TokenType type = TokenType::UNKNOWN; 
    int size = 0;                        
    bool isPrimaryKey = false;
    bool isSemantic = false;
};
struct CreateTableStmt {
    std::string tableName;
    std::vector<ColumnDef> columns;
};
struct DropTableStmt {
    std::string tableName;
};
struct DescribeTableStmt {
    std::string tableName;
};
struct InsertStmt {
    std::string tableName;
    std::vector<Value> values; 
};
struct Assignment {
    std::string column;
    Value value;
};
struct UpdateStmt {
    std::string tableName;
    std::vector<Assignment> assignments; 
    std::optional<Condition> where;
};
struct DeleteStmt {
    std::string tableName;
    std::optional<Condition> where;
};
enum class TemporalMode { NONE, AS_OF, BETWEEN, SNAPSHOT };
struct SelectStmt {
    bool selectAll = true;
    std::vector<std::string> columns; 
    std::string tableName;

    TemporalMode temporalMode = TemporalMode::NONE;
    std::optional<DateLiteral> asOfDate;     
    std::optional<DateLiteral> betweenStart; 
    std::optional<DateLiteral> betweenEnd;   
    std::optional<DateLiteral> snapshotDate; 
    std::optional<Condition> where;
    std::optional<std::string> orderByColumn;
    bool orderDescending = false;
    std::optional<int> limit;
};
struct CompareStmt {
    std::string tableName;
    Condition where; 
    DateLiteral rangeStart;
    DateLiteral rangeEnd;
};
struct EvolutionStmt {
    std::string tableName;
    Condition where; 
    DateLiteral rangeStart;
    DateLiteral rangeEnd;
};
struct HistoryStmt {
    std::string tableName;
    Condition where; 
};
struct RollbackStmt {
    std::string tableName;
    bool wholeTable = false;        
    std::optional<Condition> where;
    DateLiteral toDate;
};
struct CompactStmt {
    std::string tableName;
    DateLiteral toDate;
};
using Statement = std::variant<
    CreateTableStmt,
    DropTableStmt,
    DescribeTableStmt,
    InsertStmt,
    UpdateStmt,
    DeleteStmt,
    SelectStmt,
    CompareStmt,
    EvolutionStmt,
    HistoryStmt,
    RollbackStmt,
    CompactStmt
>;