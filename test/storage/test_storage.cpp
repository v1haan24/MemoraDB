#include <gtest/gtest.h>
#include "storage/table.h"
#include "catalog/catalog.h"
#include "../test_helper.h"
#include <cstring>

TEST(StorageTest, ValueValidation) {
    ColMeta intCol("id", DataType::INT, true);
    EXPECT_TRUE(validateValue("42", intCol));
    EXPECT_TRUE(validateValue("-100", intCol));
    EXPECT_FALSE(validateValue("abc", intCol));
    EXPECT_FALSE(validateValue("3.14", intCol));
    EXPECT_FALSE(validateValue("", intCol));

    ColMeta floatCol("price", DataType::FLOAT, false);
    EXPECT_TRUE(validateValue("19.99", floatCol));
    EXPECT_TRUE(validateValue("-0.5", floatCol));
    EXPECT_TRUE(validateValue("100", floatCol));
    EXPECT_FALSE(validateValue("xyz", floatCol));
    EXPECT_FALSE(validateValue("", floatCol));

    ColMeta strCol("name", DataType::STRING, false, 10);
    EXPECT_TRUE(validateValue("short", strCol));
    EXPECT_TRUE(validateValue("123456789", strCol)); // 9 chars <= 10 - 1
    EXPECT_FALSE(validateValue("1234567890", strCol)); // 10 chars > 9

    ColMeta boolCol("active", DataType::BOOL, false);
    EXPECT_TRUE(validateValue("true", boolCol));
    EXPECT_TRUE(validateValue("false", boolCol));
    EXPECT_FALSE(validateValue("1", boolCol));
    EXPECT_FALSE(validateValue("0", boolCol));
    EXPECT_FALSE(validateValue("True", boolCol));
}

TEST(StorageTest, InsertAndReadRecord) {
    TempDirectory tempDir;
    Catalog catalog;

    TableMeta meta;
    std::strncpy(meta.name, "items", tns - 1);
    meta.name[tns - 1] = '\0';
    meta.columns = {
        ColMeta("id", DataType::INT, true),
        ColMeta("name", DataType::STRING, false, 30),
        ColMeta("price", DataType::FLOAT, false)
    };
    ASSERT_TRUE(catalog.createTable(meta));

    Table* table = catalog.getTable("items");
    ASSERT_NE(table, nullptr);

    Row row;
    row.values = {"1", "Widget", "9.99"};
    EXPECT_TRUE(table->insert(row));

    Record rec = table->latest("1");
    EXPECT_FALSE(rec.deleted);
    EXPECT_GT(rec.timestamp, 0ULL);
    ASSERT_EQ(rec.row.values.size(), 3);
    EXPECT_EQ(rec.row.values[0], "1");
    EXPECT_EQ(rec.row.values[1], "Widget");
    EXPECT_NEAR(std::stof(rec.row.values[2]), 9.99f, 1e-4f);
}

TEST(StorageTest, DuplicateInsertFails) {
    TempDirectory tempDir;
    Catalog catalog;

    TableMeta meta;
    std::strncpy(meta.name, "items", tns - 1);
    meta.name[tns - 1] = '\0';
    meta.columns = {
        ColMeta("id", DataType::INT, true),
        ColMeta("name", DataType::STRING, false, 30)
    };
    ASSERT_TRUE(catalog.createTable(meta));

    Table* table = catalog.getTable("items");
    ASSERT_NE(table, nullptr);

    Row row1;
    row1.values = {"10", "First"};
    EXPECT_TRUE(table->insert(row1));

    Row row2;
    row2.values = {"10", "Duplicate"};
    EXPECT_FALSE(table->insert(row2));
}

TEST(StorageTest, UpdateAndReadRecord) {
    TempDirectory tempDir;
    Catalog catalog;

    TableMeta meta;
    std::strncpy(meta.name, "items", tns - 1);
    meta.name[tns - 1] = '\0';
    meta.columns = {
        ColMeta("id", DataType::INT, true),
        ColMeta("name", DataType::STRING, false, 30),
        ColMeta("price", DataType::FLOAT, false)
    };
    ASSERT_TRUE(catalog.createTable(meta));

    Table* table = catalog.getTable("items");
    ASSERT_NE(table, nullptr);

    Row row1;
    row1.values = {"1", "Widget", "9.99"};
    ASSERT_TRUE(table->insert(row1));

    Row row2;
    row2.values = {"1", "Gadget", "19.99"};
    EXPECT_TRUE(table->update(row2));

    Record rec = table->latest("1");
    EXPECT_FALSE(rec.deleted);
    ASSERT_EQ(rec.row.values.size(), 3);
    EXPECT_EQ(rec.row.values[0], "1");
    EXPECT_EQ(rec.row.values[1], "Gadget");
    EXPECT_NEAR(std::stof(rec.row.values[2]), 19.99f, 1e-4f);
}

TEST(StorageTest, DeleteRowAndPreventUpdate) {
    TempDirectory tempDir;
    Catalog catalog;

    TableMeta meta;
    std::strncpy(meta.name, "items", tns - 1);
    meta.name[tns - 1] = '\0';
    meta.columns = {
        ColMeta("id", DataType::INT, true),
        ColMeta("name", DataType::STRING, false, 30)
    };
    ASSERT_TRUE(catalog.createTable(meta));

    Table* table = catalog.getTable("items");
    ASSERT_NE(table, nullptr);

    Row row;
    row.values = {"1", "Widget"};
    ASSERT_TRUE(table->insert(row));

    EXPECT_TRUE(table->deleteRow("1"));

    Record rec = table->latest("1");
    EXPECT_TRUE(rec.deleted);

    // Cannot delete already deleted row
    EXPECT_FALSE(table->deleteRow("1"));

    // Cannot update deleted row
    Row updateRow;
    updateRow.values = {"1", "Revived"};
    EXPECT_FALSE(table->update(updateRow));
}

TEST(StorageTest, CompareRecordsDiff) {
    TableMeta meta;
    meta.columnCount = 3;
    meta.columns = {
        ColMeta("id", DataType::INT, true),
        ColMeta("name", DataType::STRING, false, 30),
        ColMeta("price", DataType::FLOAT, false)
    };

    Record r1;
    r1.timestamp = 1000;
    r1.deleted = 0;
    r1.row.values = {"1", "Widget", "9.99"};

    Record r2;
    r2.timestamp = 2000;
    r2.deleted = 0;
    r2.row.values = {"1", "Gadget", "19.99"};

    std::vector<Difference> diffs = compareRecords(r1, r2, meta);
    ASSERT_EQ(diffs.size(), 2);
    EXPECT_EQ(diffs[0].column, "name");
    EXPECT_EQ(diffs[0].before, "Widget");
    EXPECT_EQ(diffs[0].after, "Gadget");

    EXPECT_EQ(diffs[1].column, "price");
    EXPECT_EQ(diffs[1].before, "9.99");
    EXPECT_EQ(diffs[1].after, "19.99");

    Record r3;
    r3.timestamp = 3000;
    r3.deleted = 1;
    r3.row.values = {"1", "Gadget", "19.99"};

    std::vector<Difference> delDiffs = compareRecords(r2, r3, meta);
    ASSERT_EQ(delDiffs.size(), 1);
    EXPECT_EQ(delDiffs[0].column, "Deleted");
    EXPECT_EQ(delDiffs[0].before, "false");
    EXPECT_EQ(delDiffs[0].after, "true");
}
