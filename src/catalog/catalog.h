#pragma once
#include <vector>
#include <string>
#include "../types.h"
using namespace std;

class Catalog
{
    vector<TableMeta> tables;
    bool exist(const string& tableName); //Checks if a table already exists
    void CalcOffset(TableMeta& table); //Calculates the offsets for columns in a row

    public:
    bool createTable(TableMeta& table); //Writes table metadata
    TableMeta* getTable(const string& tableName); // return table metadata obj
    TableMeta readMetadata(string fileName);// Reads table metadat from disk
};