#include <gtest/gtest.h>
#include "engine/executor.h"
#include "parser/parser.h"
#include "lexer/lexer.h"
#include "../test_helper.h"
#include <thread>
#include <chrono>

static Statement parseStmt(const std::string& sql) {
    Lexer lexer(sql);
    std::vector<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    return parser.parseStatement();
}

TEST(IntegrationTest, FullWorkflowWithPersistence) {
    TempDirectory tempDir;

    auto mockEmbedder = [](const std::string& text, float (&out)[VEC_DIM]) -> bool {
        std::fill(std::begin(out), std::end(out), 0.0f);
        if (text.find("fruit") != std::string::npos) out[0] = 1.0f;
        else out[1] = 1.0f;
        return true;
    };

    uint64_t initialTimestamp = 0;

    // Phase 1: Initialize database, insert records, update, and query
    {
        Catalog catalog;
        Executor executor(catalog);
        executor.setEmbeddingProvider(mockEmbedder);

        ASSERT_TRUE(executor.execute(parseStmt(
            "CREATE TABLE warehouse (id INT PRIMARY KEY, name STRING(20), qty INT, info STRING(50) SEMANTIC)"
        )).ok());

        ASSERT_TRUE(executor.execute(parseStmt(
            "INSERT INTO warehouse VALUES (1, 'Apples', 100, 'fresh fruit')"
        )).ok());

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        Table* t = catalog.getTable("warehouse");
        ASSERT_NE(t, nullptr);
        initialTimestamp = t->latest("1").timestamp;

        ASSERT_TRUE(executor.execute(parseStmt(
            "UPDATE warehouse SET qty = 120 WHERE id = 1"
        )).ok());

        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        // Verify latest value is 120
        ExecResult selRes = executor.execute(parseStmt("SELECT qty FROM warehouse WHERE id = 1"));
        ASSERT_TRUE(selRes.ok());
        ASSERT_EQ(selRes.records.size(), 1);
        EXPECT_EQ(selRes.records[0].row.values[0], "120");

        // Verify history has 2 versions
        ExecResult histRes = executor.execute(parseStmt("HISTORY warehouse WHERE id = 1"));
        ASSERT_TRUE(histRes.ok());
        EXPECT_EQ(histRes.records.size(), 2);
    }

    // Phase 2: Open a fresh catalog instance and verify persistence
    {
        Catalog catalog2;
        Executor executor2(catalog2);
        executor2.setEmbeddingProvider(mockEmbedder);

        Table* t2 = catalog2.getTable("warehouse");
        ASSERT_NE(t2, nullptr);

        // Verify latest record survived reload
        Record rec = t2->latest("1");
        EXPECT_FALSE(rec.deleted);
        EXPECT_EQ(rec.row.values[1], "Apples");
        EXPECT_EQ(rec.row.values[2], "120");

        // Verify history survived reload
        std::vector<Record> hist = t2->showHistory("1");
        ASSERT_EQ(hist.size(), 2);
        EXPECT_EQ(hist[0].row.values[2], "100");
        EXPECT_EQ(hist[1].row.values[2], "120");

        // Rollback to initial version
        EXPECT_TRUE(t2->rollback("1", initialTimestamp));
        EXPECT_EQ(t2->latest("1").row.values[2], "100");
        EXPECT_EQ(t2->showHistory("1").size(), 3);
    }
}
