#include <gtest/gtest.h>
#include "lexer/lexer.h"
#include "parser/parser.h"

static Statement parseOne(const std::string& sql) {
    Lexer lexer(sql);
    std::vector<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    return parser.parseStatement();
}

TEST(ParserTest, CreateTable) {
    std::string sql = "CREATE TABLE users (id INT PRIMARY KEY, name STRING(50), bio STRING(256) SEMANTIC, active BOOL)";
    Statement stmt = parseOne(sql);

    ASSERT_TRUE(std::holds_alternative<CreateTableStmt>(stmt));
    const auto& create = std::get<CreateTableStmt>(stmt);
    EXPECT_EQ(create.tableName, "users");
    ASSERT_EQ(create.columns.size(), 4);

    EXPECT_EQ(create.columns[0].name, "id");
    EXPECT_EQ(create.columns[0].type, TokenType::INT);
    EXPECT_TRUE(create.columns[0].isPrimaryKey);
    EXPECT_FALSE(create.columns[0].isSemantic);

    EXPECT_EQ(create.columns[1].name, "name");
    EXPECT_EQ(create.columns[1].type, TokenType::STRING);
    EXPECT_EQ(create.columns[1].size, 50);
    EXPECT_FALSE(create.columns[1].isPrimaryKey);

    EXPECT_EQ(create.columns[2].name, "bio");
    EXPECT_EQ(create.columns[2].type, TokenType::STRING);
    EXPECT_EQ(create.columns[2].size, 256);
    EXPECT_TRUE(create.columns[2].isSemantic);

    EXPECT_EQ(create.columns[3].name, "active");
    EXPECT_EQ(create.columns[3].type, TokenType::BOOL);
}

TEST(ParserTest, CreateTableInvalidSizeThrows) {
    EXPECT_THROW(parseOne("CREATE TABLE bad (id INT(10))"), ParseError);
}

TEST(ParserTest, DropAndDescribeAndShowTables) {
    Statement dropStmt = parseOne("DROP TABLE users");
    ASSERT_TRUE(std::holds_alternative<DropTableStmt>(dropStmt));
    EXPECT_EQ(std::get<DropTableStmt>(dropStmt).tableName, "users");

    Statement descStmt = parseOne("DESCRIBE TABLE users");
    ASSERT_TRUE(std::holds_alternative<DescribeTableStmt>(descStmt));
    EXPECT_EQ(std::get<DescribeTableStmt>(descStmt).tableName, "users");

    Statement showStmt = parseOne("SHOW TABLES");
    ASSERT_TRUE(std::holds_alternative<ShowTablesStmt>(showStmt));
}

TEST(ParserTest, InsertStatement) {
    Statement stmt = parseOne("INSERT INTO users VALUES (1, 'Alice', -3.14)");
    ASSERT_TRUE(std::holds_alternative<InsertStmt>(stmt));
    const auto& insert = std::get<InsertStmt>(stmt);

    EXPECT_EQ(insert.tableName, "users");
    ASSERT_EQ(insert.values.size(), 3);
    EXPECT_EQ(insert.values[0].kind, Value::Kind::INT);
    EXPECT_EQ(insert.values[0].intVal, 1);
    EXPECT_EQ(insert.values[1].kind, Value::Kind::STRING);
    EXPECT_EQ(insert.values[1].strVal, "Alice");
    EXPECT_EQ(insert.values[2].kind, Value::Kind::FLOAT);
    EXPECT_DOUBLE_EQ(insert.values[2].floatVal, -3.14);
}

TEST(ParserTest, UpdateStatement) {
    Statement stmt = parseOne("UPDATE users SET name = 'Bob', age = 30 WHERE id = 1");
    ASSERT_TRUE(std::holds_alternative<UpdateStmt>(stmt));
    const auto& update = std::get<UpdateStmt>(stmt);

    EXPECT_EQ(update.tableName, "users");
    ASSERT_EQ(update.assignments.size(), 2);
    EXPECT_EQ(update.assignments[0].column, "name");
    EXPECT_EQ(update.assignments[0].value.strVal, "Bob");
    EXPECT_EQ(update.assignments[1].column, "age");
    EXPECT_EQ(update.assignments[1].value.intVal, 30);
    ASSERT_TRUE(update.where.has_value());
    EXPECT_EQ(update.where->column, "id");
    EXPECT_EQ(update.where->op, CompareOp::EQ);
    EXPECT_EQ(update.where->value.intVal, 1);
}

TEST(ParserTest, DeleteStatement) {
    Statement stmt = parseOne("DELETE FROM users WHERE id = 1");
    ASSERT_TRUE(std::holds_alternative<DeleteStmt>(stmt));
    const auto& del = std::get<DeleteStmt>(stmt);

    EXPECT_EQ(del.tableName, "users");
    ASSERT_TRUE(del.where.has_value());
    EXPECT_EQ(del.where->column, "id");
}

TEST(ParserTest, SelectStatementFull) {
    Statement stmt = parseOne("SELECT id, name FROM users WHERE id > 5 ORDER BY id DESC LIMIT 10");
    ASSERT_TRUE(std::holds_alternative<SelectStmt>(stmt));
    const auto& sel = std::get<SelectStmt>(stmt);

    EXPECT_FALSE(sel.selectAll);
    ASSERT_EQ(sel.columns.size(), 2);
    EXPECT_EQ(sel.columns[0], "id");
    EXPECT_EQ(sel.columns[1], "name");
    EXPECT_EQ(sel.tableName, "users");
    ASSERT_TRUE(sel.where.has_value());
    EXPECT_EQ(sel.where->column, "id");
    EXPECT_EQ(sel.where->op, CompareOp::GT);
    EXPECT_EQ(sel.where->value.intVal, 5);
    ASSERT_TRUE(sel.orderByColumn.has_value());
    EXPECT_EQ(sel.orderByColumn.value(), "id");
    EXPECT_TRUE(sel.orderDescending);
    ASSERT_TRUE(sel.limit.has_value());
    EXPECT_EQ(sel.limit.value(), 10);
}

TEST(ParserTest, SelectTemporalModes) {
    Statement asOfStmt = parseOne("SELECT * FROM users AS OF 2026-03-15 10:30:00");
    ASSERT_TRUE(std::holds_alternative<SelectStmt>(asOfStmt));
    const auto& sel1 = std::get<SelectStmt>(asOfStmt);
    EXPECT_EQ(sel1.temporalMode, TemporalMode::AS_OF);
    ASSERT_TRUE(sel1.asOfDate.has_value());
    EXPECT_EQ(sel1.asOfDate->year, 2026);
    EXPECT_EQ(sel1.asOfDate->month, 3);
    EXPECT_EQ(sel1.asOfDate->day, 15);
    EXPECT_EQ(sel1.asOfDate->hour, 10);
    EXPECT_EQ(sel1.asOfDate->minute, 30);

    Statement betweenStmt = parseOne("SELECT * FROM users BETWEEN 2026-01-01 AND 2026-02-01");
    ASSERT_TRUE(std::holds_alternative<SelectStmt>(betweenStmt));
    const auto& sel2 = std::get<SelectStmt>(betweenStmt);
    EXPECT_EQ(sel2.temporalMode, TemporalMode::BETWEEN);
    ASSERT_TRUE(sel2.betweenStart.has_value());
    ASSERT_TRUE(sel2.betweenEnd.has_value());

    Statement snapshotStmt = parseOne("SELECT * FROM users SNAPSHOT 2026-01-01");
    ASSERT_TRUE(std::holds_alternative<SelectStmt>(snapshotStmt));
    const auto& sel3 = std::get<SelectStmt>(snapshotStmt);
    EXPECT_EQ(sel3.temporalMode, TemporalMode::SNAPSHOT);
    ASSERT_TRUE(sel3.snapshotDate.has_value());
}

TEST(ParserTest, SelectSemanticSimilarTo) {
    Statement stmt = parseOne("SELECT * FROM docs WHERE content SIMILAR TO \"artificial intelligence\" LIMIT 5");
    ASSERT_TRUE(std::holds_alternative<SelectStmt>(stmt));
    const auto& sel = std::get<SelectStmt>(stmt);

    ASSERT_TRUE(sel.where.has_value());
    EXPECT_EQ(sel.where->column, "content");
    EXPECT_EQ(sel.where->op, CompareOp::SIMILAR_TO);
    EXPECT_EQ(sel.where->value.strVal, "artificial intelligence");
    ASSERT_TRUE(sel.limit.has_value());
    EXPECT_EQ(sel.limit.value(), 5);
}

TEST(ParserTest, TemporalStatements) {
    Statement compStmt = parseOne("COMPARE users WHERE id = 1 BETWEEN 2026-01-01 AND 2026-02-01");
    ASSERT_TRUE(std::holds_alternative<CompareStmt>(compStmt));
    const auto& comp = std::get<CompareStmt>(compStmt);
    EXPECT_EQ(comp.tableName, "users");
    EXPECT_EQ(comp.where.column, "id");

    Statement evoStmt = parseOne("EVOLUTION users WHERE id = 1 BETWEEN 2026-01-01 AND 2026-02-01");
    ASSERT_TRUE(std::holds_alternative<EvolutionStmt>(evoStmt));

    Statement histStmt = parseOne("HISTORY users WHERE id = 1");
    ASSERT_TRUE(std::holds_alternative<HistoryStmt>(histStmt));

    Statement rbRowStmt = parseOne("ROLLBACK users WHERE id = 1 TO 2026-01-01");
    ASSERT_TRUE(std::holds_alternative<RollbackStmt>(rbRowStmt));
    EXPECT_FALSE(std::get<RollbackStmt>(rbRowStmt).wholeTable);

    Statement rbTblStmt = parseOne("ROLLBACK TABLE users TO 2026-01-01");
    ASSERT_TRUE(std::holds_alternative<RollbackStmt>(rbTblStmt));
    EXPECT_TRUE(std::get<RollbackStmt>(rbTblStmt).wholeTable);

    Statement compTableStmt = parseOne("COMPACT TABLE users TO 2026-01-01");
    ASSERT_TRUE(std::holds_alternative<CompactStmt>(compTableStmt));
}

TEST(ParserTest, ParseProgramMultipleStatements) {
    std::string sql = "CREATE TABLE t (id INT PRIMARY KEY); INSERT INTO t VALUES (1);";
    Lexer lexer(sql);
    std::vector<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    std::vector<Statement> stmts = parser.parseProgram();

    ASSERT_EQ(stmts.size(), 2);
    EXPECT_TRUE(std::holds_alternative<CreateTableStmt>(stmts[0]));
    EXPECT_TRUE(std::holds_alternative<InsertStmt>(stmts[1]));
}

TEST(ParserTest, SyntaxErrorThrows) {
    EXPECT_THROW(parseOne("SELECT FROM"), ParseError);
    EXPECT_THROW(parseOne("INSERT INTO"), ParseError);
    EXPECT_THROW(parseOne("FOOBAR"), ParseError);
}
