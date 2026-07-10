#pragma once
#include "../types.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>

class Table {
private:
    TableStruct meta;
    std::unordered_map<std::string, std::vector<RowVersion>> pkIndex; 

    std::string getPK(const DataRow& row);
    bool validateRow(const DataRow& row);
    void writeFixedString(std::ofstream& file, const std::string& val, int sizeBytes);
    
    // crucial helper to safely check strings before doing stoi/stof
    bool isIntStr(const std::string& s);
    bool isFloatStr(const std::string& s);

    // handles RAM index rebuilding on server startup
    void recoverState();

public:
    Table(const TableStruct& tableMeta);
    bool insertRecord(const DataRow& row);
    int getRowCount() const;
};