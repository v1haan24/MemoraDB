#include "catalog.h"
#include "../storage/serializer.h"
#include <iostream>
#include <unordered_set>

using namespace std;

bool DatabaseCatalog::checkExists(string name) {
    for (int i = 0; i < allTables.size(); i++) {
        if (strcmp(allTables[i].tblName, name.c_str()) == 0) return true;
    }
    return false;
}

void DatabaseCatalog::computeOffsets(TableStruct& t) {
    int currentSize = 0;
    for (int i = 0; i < t.colList.size(); i++) {
        t.colList[i].offsetValue = currentSize;
        currentSize += t.colList[i].sizeBytes;
    }
    t.recordSize = currentSize;
}

bool DatabaseCatalog::hasSpace(const char* str) {
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] == ' ') return true;
    }
    return false;
}

bool DatabaseCatalog::makeNewTable(TableStruct& t) {
    if (strlen(t.tblName) >= tns || t.colList.size() == 0 || checkExists(t.tblName)) return false;
    if (hasSpace(t.tblName)) return false;

    unordered_set<string> nameChecker;
    int pkCounter = 0;

    for (int i = 0; i < t.colList.size(); i++) {
        ColumnInfo& c = t.colList[i];
        if (strlen(c.colName) >= cns || hasSpace(c.colName)) return false;
        if (nameChecker.insert(c.colName).second == false) return false;
        if (c.dType == STRING && c.sizeBytes <= 0) return false;
        if (c.isPrimaryKey && c.dType == BOOL) return false;
        if (c.isPrimaryKey) pkCounter++;
    }

    if (pkCounter != 1) return false;
    computeOffsets(t);
    if (t.recordSize == 0) return false;

    string fileName = "data/" + string(t.tblName) + ".db";
    ofstream file(fileName, ios::binary);
    if (!file) return false;

    t.totalCols = t.colList.size();
    t.metaSize = 46 + 43 * t.totalCols; 

    writeBinary(file, t.metaSize);
    file.write(t.tblName, tns);
    writeBinary(file, t.totalRows);
    writeBinary(file, t.recordSize);
    writeBinary(file, t.totalCols);

    for (int i = 0; i < t.totalCols; i++) {
        writeColumnInfo(file, t.colList[i]);
    }

    allTables.push_back(t);
    file.close();
    return true;
}

TableStruct DatabaseCatalog::loadTable(string fileName) {
    string fullPath = "data/" + fileName;
    ifstream file(fullPath, ios::binary);
    TableStruct t;
    
    if (!file) {
        cout << "Error: Could not open schema file.\n";
        return t; 
    }

    readBinary(file, t.metaSize);
    file.read(t.tblName, tns);
    readBinary(file, t.totalRows);
    readBinary(file, t.recordSize);
    readBinary(file, t.totalCols);

    for (int i = 0; i < t.totalCols; i++) {
        ColumnInfo c;
        readColumnInfo(file, c);
        t.colList.push_back(c);
    }
    
    file.close();
    return t;
}