#include <gtest/gtest.h>
#include "catalog/catalog.h"
#include "../test_helper.h"
#include <cstring>

TEST(CatalogTest, CreateAndGetTable) {
    TempDirectory tempDir;
    Catalog catalog;

    TableMeta meta;
    std::strncpy(meta.name, "users", tns - 1);
    meta.name[tns - 1] = '\0';
    meta.columns = {
        ColMeta("id", DataType::INT, true),
        ColMeta("name", DataType::STRING, false, 50),
        ColMeta("score", DataType::FLOAT, false)
    };

    EXPECT_TRUE(catalog.createTable(meta));
    Table* table = catalog.getTable("users");
    ASSERT_NE(table, nullptr);
    EXPECT_STREQ(table->getMeta().name, "users");
    EXPECT_EQ(table->getMeta().columnCount, 3);
}

TEST(CatalogTest, DuplicateTableFails) {
    TempDirectory tempDir;
    Catalog catalog;

    TableMeta meta;
    std::strncpy(meta.name, "items", tns - 1);
    meta.name[tns - 1] = '\0';
    meta.columns = {
        ColMeta("id", DataType::INT, true),
        ColMeta("title", DataType::STRING, false, 30)
    };

    EXPECT_TRUE(catalog.createTable(meta));
    EXPECT_FALSE(catalog.createTable(meta));
}

TEST(CatalogTest, InvalidColumnConfigurations) {
    TempDirectory tempDir;
    Catalog catalog;

    // No columns
    {
        TableMeta meta;
        std::strncpy(meta.name, "empty_cols", tns - 1);
        meta.name[tns - 1] = '\0';
        EXPECT_FALSE(catalog.createTable(meta));
    }

    // Duplicate column names
    {
        TableMeta meta;
        std::strncpy(meta.name, "dup_cols", tns - 1);
        meta.name[tns - 1] = '\0';
        meta.columns = {
            ColMeta("id", DataType::INT, true),
            ColMeta("id", DataType::STRING, false, 20)
        };
        EXPECT_FALSE(catalog.createTable(meta));
    }

    // No primary key
    {
        TableMeta meta;
        std::strncpy(meta.name, "no_pk", tns - 1);
        meta.name[tns - 1] = '\0';
        meta.columns = {
            ColMeta("id", DataType::INT, false),
            ColMeta("name", DataType::STRING, false, 20)
        };
        EXPECT_FALSE(catalog.createTable(meta));
    }

    // Multiple primary keys
    {
        TableMeta meta;
        std::strncpy(meta.name, "multi_pk", tns - 1);
        meta.name[tns - 1] = '\0';
        meta.columns = {
            ColMeta("id1", DataType::INT, true),
            ColMeta("id2", DataType::INT, true)
        };
        EXPECT_FALSE(catalog.createTable(meta));
    }

    // String column with non-positive size
    {
        TableMeta meta;
        std::strncpy(meta.name, "bad_str", tns - 1);
        meta.name[tns - 1] = '\0';
        meta.columns = {
            ColMeta("id", DataType::INT, true),
            ColMeta("desc", DataType::STRING, false, 0)
        };
        EXPECT_FALSE(catalog.createTable(meta));
    }
}

TEST(CatalogTest, DropTable) {
    TempDirectory tempDir;
    Catalog catalog;

    TableMeta meta;
    std::strncpy(meta.name, "to_drop", tns - 1);
    meta.name[tns - 1] = '\0';
    meta.columns = {
        ColMeta("id", DataType::INT, true)
    };

    EXPECT_TRUE(catalog.createTable(meta));
    EXPECT_NE(catalog.getTable("to_drop"), nullptr);

    EXPECT_TRUE(catalog.dropTable("to_drop"));
    EXPECT_EQ(catalog.getTable("to_drop"), nullptr);
    EXPECT_FALSE(catalog.dropTable("to_drop"));
}

TEST(CatalogTest, ReloadTablesFromDisk) {
    TempDirectory tempDir;

    {
        Catalog catalog;
        TableMeta meta;
        std::strncpy(meta.name, "persisted", tns - 1);
        meta.name[tns - 1] = '\0';
        meta.columns = {
            ColMeta("id", DataType::INT, true),
            ColMeta("label", DataType::STRING, false, 40)
        };
        ASSERT_TRUE(catalog.createTable(meta));
    }

    {
        Catalog reloaded;
        Table* table = reloaded.getTable("persisted");
        ASSERT_NE(table, nullptr);
        EXPECT_STREQ(table->getMeta().name, "persisted");
        EXPECT_EQ(table->getMeta().columnCount, 2);
    }
}
