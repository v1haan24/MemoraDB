#include <gtest/gtest.h>
#include "storage/table.h"
#include "catalog/catalog.h"
#include "../test_helper.h"
#include <thread>
#include <chrono>
#include <cstring>

TEST(TemporalTest, HistoryTracking) {
    TempDirectory tempDir;
    Catalog catalog;

    TableMeta meta;
    std::strncpy(meta.name, "accounts", tns - 1);
    meta.name[tns - 1] = '\0';
    meta.columns = {
        ColMeta("id", DataType::INT, true),
        ColMeta("balance", DataType::INT, false)
    };
    ASSERT_TRUE(catalog.createTable(meta));
    Table* table = catalog.getTable("accounts");
    ASSERT_NE(table, nullptr);

    Row r1; r1.values = {"1", "100"};
    ASSERT_TRUE(table->insert(r1));
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    Row r2; r2.values = {"1", "200"};
    ASSERT_TRUE(table->update(r2));
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    Row r3; r3.values = {"1", "300"};
    ASSERT_TRUE(table->update(r3));

    std::vector<Record> hist = table->showHistory("1");
    ASSERT_EQ(hist.size(), 3);
    EXPECT_EQ(hist[0].row.values[1], "100");
    EXPECT_EQ(hist[1].row.values[1], "200");
    EXPECT_EQ(hist[2].row.values[1], "300");
    EXPECT_LT(hist[0].timestamp, hist[1].timestamp);
    EXPECT_LT(hist[1].timestamp, hist[2].timestamp);
}

TEST(TemporalTest, SelectAsOf) {
    TempDirectory tempDir;
    Catalog catalog;

    TableMeta meta;
    std::strncpy(meta.name, "accounts", tns - 1);
    meta.name[tns - 1] = '\0';
    meta.columns = {
        ColMeta("id", DataType::INT, true),
        ColMeta("balance", DataType::INT, false)
    };
    ASSERT_TRUE(catalog.createTable(meta));
    Table* table = catalog.getTable("accounts");
    ASSERT_NE(table, nullptr);

    Row r1; r1.values = {"1", "100"};
    ASSERT_TRUE(table->insert(r1));
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    Row r2; r2.values = {"1", "200"};
    ASSERT_TRUE(table->update(r2));
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    Row r3; r3.values = {"1", "300"};
    ASSERT_TRUE(table->update(r3));

    std::vector<Record> hist = table->showHistory("1");
    ASSERT_EQ(hist.size(), 3);
    uint64_t t1 = hist[0].timestamp;
    uint64_t t2 = hist[1].timestamp;
    uint64_t t3 = hist[2].timestamp;

    Record asOf1 = table->selectAsOf("1", t1);
    EXPECT_EQ(asOf1.row.values[1], "100");

    Record asOf2 = table->selectAsOf("1", t2);
    EXPECT_EQ(asOf2.row.values[1], "200");

    Record asOf3 = table->selectAsOf("1", t3);
    EXPECT_EQ(asOf3.row.values[1], "300");

    Record asOfBefore = table->selectAsOf("1", t1 - 100);
    EXPECT_TRUE(asOfBefore.row.values.empty());
}

TEST(TemporalTest, SelectBetween) {
    TempDirectory tempDir;
    Catalog catalog;

    TableMeta meta;
    std::strncpy(meta.name, "accounts", tns - 1);
    meta.name[tns - 1] = '\0';
    meta.columns = {
        ColMeta("id", DataType::INT, true),
        ColMeta("balance", DataType::INT, false)
    };
    ASSERT_TRUE(catalog.createTable(meta));
    Table* table = catalog.getTable("accounts");
    ASSERT_NE(table, nullptr);

    Row r1; r1.values = {"1", "100"};
    ASSERT_TRUE(table->insert(r1));
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    Row r2; r2.values = {"1", "200"};
    ASSERT_TRUE(table->update(r2));
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    Row r3; r3.values = {"1", "300"};
    ASSERT_TRUE(table->update(r3));

    std::vector<Record> hist = table->showHistory("1");
    ASSERT_EQ(hist.size(), 3);
    uint64_t t1 = hist[0].timestamp;
    uint64_t t2 = hist[1].timestamp;
    uint64_t t3 = hist[2].timestamp;

    std::vector<Record> between12 = table->selectBetween("1", t1, t2);
    ASSERT_EQ(between12.size(), 2);
    EXPECT_EQ(between12[0].row.values[1], "100");
    EXPECT_EQ(between12[1].row.values[1], "200");

    std::vector<Record> between23 = table->selectBetween("1", t2, t3);
    ASSERT_EQ(between23.size(), 2);
    EXPECT_EQ(between23[0].row.values[1], "200");
    EXPECT_EQ(between23[1].row.values[1], "300");
}

TEST(TemporalTest, SnapshotPointInTime) {
    TempDirectory tempDir;
    Catalog catalog;

    TableMeta meta;
    std::strncpy(meta.name, "accounts", tns - 1);
    meta.name[tns - 1] = '\0';
    meta.columns = {
        ColMeta("id", DataType::INT, true),
        ColMeta("balance", DataType::INT, false)
    };
    ASSERT_TRUE(catalog.createTable(meta));
    Table* table = catalog.getTable("accounts");
    ASSERT_NE(table, nullptr);

    Row r1; r1.values = {"1", "100"};
    ASSERT_TRUE(table->insert(r1));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    uint64_t snap1Time = table->latest("1").timestamp;

    Row r2; r2.values = {"2", "500"};
    ASSERT_TRUE(table->insert(r2));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    uint64_t snap2Time = table->latest("2").timestamp;

    std::vector<Record> snap1 = table->snapshot(snap1Time);
    ASSERT_EQ(snap1.size(), 1);
    EXPECT_EQ(snap1[0].row.values[0], "1");

    std::vector<Record> snap2 = table->snapshot(snap2Time);
    EXPECT_EQ(snap2.size(), 2);

    ASSERT_TRUE(table->deleteRow("1"));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    uint64_t snap3Time = table->latest("1").timestamp;

    std::vector<Record> snap3 = table->snapshot(snap3Time);
    ASSERT_EQ(snap3.size(), 1);
    EXPECT_EQ(snap3[0].row.values[0], "2");

    // Old snapshot still shows user 1
    std::vector<Record> snap1Again = table->snapshot(snap1Time);
    ASSERT_EQ(snap1Again.size(), 1);
    EXPECT_EQ(snap1Again[0].row.values[0], "1");
}

TEST(TemporalTest, CompareAndEvolution) {
    TempDirectory tempDir;
    Catalog catalog;

    TableMeta meta;
    std::strncpy(meta.name, "items", tns - 1);
    meta.name[tns - 1] = '\0';
    meta.columns = {
        ColMeta("id", DataType::INT, true),
        ColMeta("status", DataType::STRING, false, 20)
    };
    ASSERT_TRUE(catalog.createTable(meta));
    Table* table = catalog.getTable("items");
    ASSERT_NE(table, nullptr);

    Row r1; r1.values = {"1", "pending"};
    ASSERT_TRUE(table->insert(r1));
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    Row r2; r2.values = {"1", "processing"};
    ASSERT_TRUE(table->update(r2));
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    Row r3; r3.values = {"1", "completed"};
    ASSERT_TRUE(table->update(r3));

    std::vector<Record> hist = table->showHistory("1");
    ASSERT_EQ(hist.size(), 3);
    uint64_t t1 = hist[0].timestamp;
    uint64_t t3 = hist[2].timestamp;

    std::vector<Difference> diff = table->compare("1", t1, t3);
    ASSERT_EQ(diff.size(), 1);
    EXPECT_EQ(diff[0].column, "status");
    EXPECT_EQ(diff[0].before, "pending");
    EXPECT_EQ(diff[0].after, "completed");

    std::vector<Difference> evo = table->evolution("1", t1, t3);
    ASSERT_EQ(evo.size(), 2);
    EXPECT_EQ(evo[0].before, "pending");
    EXPECT_EQ(evo[0].after, "processing");
    EXPECT_EQ(evo[1].before, "processing");
    EXPECT_EQ(evo[1].after, "completed");
}

TEST(TemporalTest, RollbackRow) {
    TempDirectory tempDir;
    Catalog catalog;

    TableMeta meta;
    std::strncpy(meta.name, "settings", tns - 1);
    meta.name[tns - 1] = '\0';
    meta.columns = {
        ColMeta("id", DataType::INT, true),
        ColMeta("theme", DataType::STRING, false, 20)
    };
    ASSERT_TRUE(catalog.createTable(meta));
    Table* table = catalog.getTable("settings");
    ASSERT_NE(table, nullptr);

    Row r1; r1.values = {"1", "dark"};
    ASSERT_TRUE(table->insert(r1));
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    uint64_t t1 = table->latest("1").timestamp;

    Row r2; r2.values = {"1", "light"};
    ASSERT_TRUE(table->update(r2));
    EXPECT_EQ(table->latest("1").row.values[1], "light");

    EXPECT_TRUE(table->rollback("1", t1));
    EXPECT_EQ(table->latest("1").row.values[1], "dark");
    EXPECT_EQ(table->showHistory("1").size(), 3);
}
