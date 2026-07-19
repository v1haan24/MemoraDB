#pragma once
#include <unordered_map>
#include <string>
#include "../common/metadata.h"
#include "../storage/table.h"
using namespace std;

class Catalog{
private:
    unordered_map<string, Table> tables;
    bool exist(const string& tableName);
    void CalcOffset(TableMeta& table);
    TableMeta readMetadata(const string& fileName);
    void loadTables();
public:
    Catalog(){loadTables();}
    bool createTable(TableMeta& table);
    Table* getTable(const string& tableName);
    void showTables();
};