#include <gtest/gtest.h>
#include "engine/executor.h"
#include "parser/parser.h"
#include "lexer/lexer.h"
#include "../test_helper.h"

static Statement parseStmt(const std::string& sql) {
    Lexer lexer(sql);
    std::vector<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    return parser.parseStatement();
}

TEST(ExecutorTest, DDLExecution) {
    TempDirectory tempDir;
    Catalog catalog;
    Executor executor(catalog);

    ExecResult res = executor.execute(parseStmt(
        "CREATE TABLE users (id INT PRIMARY KEY, name STRING(30), score FLOAT)"
    ));
    EXPECT_TRUE(res.ok());
    EXPECT_EQ(res.kind, ExecResult::Kind::OK);

    ExecResult descRes = executor.execute(parseStmt("DESCRIBE TABLE users"));
    EXPECT_TRUE(descRes.ok());

    ExecResult showRes = executor.execute(parseStmt("SHOW TABLES"));
    EXPECT_TRUE(showRes.ok());
    EXPECT_NE(showRes.message.find("users"), std::string::npos);

    ExecResult dropRes = executor.execute(parseStmt("DROP TABLE users"));
    EXPECT_TRUE(dropRes.ok());

    ExecResult reDropRes = executor.execute(parseStmt("DROP TABLE users"));
    EXPECT_FALSE(reDropRes.ok());
}

TEST(ExecutorTest, DMLExecution) {
    TempDirectory tempDir;
    Catalog catalog;
    Executor executor(catalog);

    ASSERT_TRUE(executor.execute(parseStmt(
        "CREATE TABLE employees (id INT PRIMARY KEY, name STRING(30), salary INT)"
    )).ok());

    // Insert 2 rows
    EXPECT_TRUE(executor.execute(parseStmt(
        "INSERT INTO employees VALUES (1, 'Alice', 50000)"
    )).ok());
    EXPECT_TRUE(executor.execute(parseStmt(
        "INSERT INTO employees VALUES (2, 'Bob', 60000)"
    )).ok());

    // Duplicate insert fails
    EXPECT_FALSE(executor.execute(parseStmt(
        "INSERT INTO employees VALUES (1, 'Duplicate', 70000)"
    )).ok());

    // Update with WHERE
    ExecResult updateRes = executor.execute(parseStmt(
        "UPDATE employees SET salary = 55000 WHERE id = 1"
    ));
    EXPECT_TRUE(updateRes.ok());
    EXPECT_EQ(updateRes.message, "1 row(s) updated");

    // Updating PK is prohibited
    ExecResult badUpdate = executor.execute(parseStmt(
        "UPDATE employees SET id = 10 WHERE id = 1"
    ));
    EXPECT_FALSE(badUpdate.ok());

    // Delete with WHERE
    ExecResult delRes = executor.execute(parseStmt(
        "DELETE FROM employees WHERE id = 2"
    ));
    EXPECT_TRUE(delRes.ok());
    EXPECT_EQ(delRes.message, "1 row(s) deleted");
}

TEST(ExecutorTest, SelectFilteringSortingAndProjection) {
    TempDirectory tempDir;
    Catalog catalog;
    Executor executor(catalog);

    ASSERT_TRUE(executor.execute(parseStmt(
        "CREATE TABLE students (id INT PRIMARY KEY, name STRING(20), grade INT)"
    )).ok());

    ASSERT_TRUE(executor.execute(parseStmt("INSERT INTO students VALUES (1, 'Alice', 85)")).ok());
    ASSERT_TRUE(executor.execute(parseStmt("INSERT INTO students VALUES (2, 'Bob', 92)")).ok());
    ASSERT_TRUE(executor.execute(parseStmt("INSERT INTO students VALUES (3, 'Charlie', 78)")).ok());
    ASSERT_TRUE(executor.execute(parseStmt("INSERT INTO students VALUES (4, 'David', 95)")).ok());

    // SELECT with projection, WHERE, ORDER BY, LIMIT
    ExecResult selRes = executor.execute(parseStmt(
        "SELECT name, grade FROM students WHERE grade >= 85 ORDER BY grade DESC LIMIT 2"
    ));

    ASSERT_TRUE(selRes.ok());
    EXPECT_EQ(selRes.kind, ExecResult::Kind::ROWS);
    ASSERT_EQ(selRes.columns.size(), 2);
    EXPECT_STREQ(selRes.columns[0].name, "name");
    EXPECT_STREQ(selRes.columns[1].name, "grade");

    ASSERT_EQ(selRes.records.size(), 2);
    // Highest grade is David (95)
    EXPECT_EQ(selRes.records[0].row.values[0], "David");
    EXPECT_EQ(selRes.records[0].row.values[1], "95");
    // Second highest is Bob (92)
    EXPECT_EQ(selRes.records[1].row.values[0], "Bob");
    EXPECT_EQ(selRes.records[1].row.values[1], "92");
}

TEST(ExecutorTest, NonExistentTableFails) {
    TempDirectory tempDir;
    Catalog catalog;
    Executor executor(catalog);

    ExecResult res = executor.execute(parseStmt("SELECT * FROM ghost_table"));
    EXPECT_FALSE(res.ok());
    EXPECT_NE(res.message.find("No such table"), std::string::npos);
}
