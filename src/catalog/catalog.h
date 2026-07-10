#pragma once
#include <vector>
#include <string>
#include "../types.h"
using namespace std;

class Catalog{
    vector<TableMeta> tables;
    bool exist(const string& tableName);
    void CalcOffset(TableMeta& table);
public:
    bool createTable(TableMeta& table);
    TableMeta* getTable(const string& tableName);
    TableMeta readMetadata(string fileName);
};