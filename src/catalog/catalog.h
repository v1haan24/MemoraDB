#pragma once
#include <unordered_map>
#include <string>
#include "../common/metadata.h"
#include "../storage/table.h"

class Catalog{
private:
    std::unordered_map<std::string, Table> tables;
    bool exist(const std::string& tableName);
    void CalcOffset(TableMeta& table);
    TableMeta readMetadata(const std::string& fileName);
    void loadTables();
public:
    Catalog(){loadTables();}
    bool createTable(TableMeta& table);
    Table* getTable(const std::string& tableName);
    void showTables();
};