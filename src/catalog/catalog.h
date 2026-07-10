#pragma once
#include <string>
#include <vector>
#include "../types.h"

class DatabaseCatalog {
private:
    std::vector<TableStruct> allTables;
    
    bool checkExists(std::string name);
    void computeOffsets(TableStruct& t);
    bool hasSpace(const char* str);

public:
    bool makeNewTable(TableStruct& t);
    TableStruct* fetchTable(const std::string& tblName);
    TableStruct loadTable(std::string fileName);
};