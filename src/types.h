#pragma once
#include <vector>
#include <string>
#include <cstring>

// fixed sizes to keep binary files aligned perfectly
#define cns 30 
#define tns 30 

enum DataType { INT, FLOAT, STRING, BOOL };

struct ColumnInfo {
    char colName[cns];
    bool isPrimaryKey;
    DataType dType;
    int sizeBytes;
    int offsetValue = 0;

    ColumnInfo() = default; 
    ColumnInfo(std::string n, DataType t, bool pk, int s = 0) {
        strncpy(colName, n.c_str(), cns - 1);
        colName[cns - 1] = '\0';
        dType = t;
        isPrimaryKey = pk;
        
        if (dType == STRING) sizeBytes = s;
        else if (dType == INT) sizeBytes = sizeof(int);
        else if (dType == FLOAT) sizeBytes = sizeof(float);
        else if (dType == BOOL) sizeBytes = sizeof(bool);
    }
};

struct TableStruct {
    int metaSize;
    std::vector<ColumnInfo> colList;
    int totalRows = 0;
    int totalCols = 0;
    char tblName[tns];
    int recordSize = 0;
};

// struct to hold the incoming row data from the parser
struct DataRow {
    std::vector<std::string> values;
};

// keeping track of timestamps and byte offsets for the ram index
struct RowVersion {
    uint64_t timestamp;
    uint64_t offset;
};