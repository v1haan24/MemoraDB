#include "table.h"
#include "serializer.h"
#include <chrono>
#include <cctype>

using namespace std;

bool Table::isIntStr(const string& s) {
    if (s.empty()) return false;
    size_t start = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    if (start == s.length()) return false;
    for (size_t i = start; i < s.length(); i++) {
        if (!isdigit(s[i])) return false;
    }
    return true;
}

bool Table::isFloatStr(const string& s) {
    if (s.empty()) return false;
    size_t start = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    bool hasDot = false;
    bool hasDigit = false;
    for (size_t i = start; i < s.length(); i++) {
        if (isdigit(s[i])) hasDigit = true;
        else if (s[i] == '.' && !hasDot) hasDot = true;
        else return false;
    }
    return hasDigit;
}

string Table::getPK(const DataRow& row) {
    for (int i = 0; i < meta.totalCols; i++) {
        if (meta.colList[i].isPrimaryKey) return row.values[i];
    }
    return "";
}

bool Table::validateRow(const DataRow& row) {
    if (row.values.size() != meta.totalCols) return false;
    for (int i = 0; i < meta.totalCols; i++) {
        DataType type = meta.colList[i].dType;
        const string& val = row.values[i];
        
        if (type == INT && !isIntStr(val)) return false;
        if (type == FLOAT && !isFloatStr(val)) return false;
        if (type == BOOL && val != "true" && val != "false") return false;
        if (type == STRING && val.length() > meta.colList[i].sizeBytes) return false;
    }
    return true;
}

void Table::writeFixedString(ofstream& file, const string& val, int sizeBytes) {
    vector<char> buffer(sizeBytes, '\0');
    memcpy(buffer.data(), val.data(), min((int)val.size(), sizeBytes - 1));
    file.write(buffer.data(), sizeBytes);
}

void Table::recoverState() {
    string fileName = "data/" + string(meta.tblName) + "_data.dat";
    ifstream dataFile(fileName, ios::binary);
    if (!dataFile) return; // if file isn't there yet, just start fresh

    meta.totalRows = 0;
    while (dataFile.peek() != EOF) {
        uint64_t offset = dataFile.tellg();
        uint64_t timestamp;
        bool isDeleted;
        
        readBinary(dataFile, timestamp);
        readBinary(dataFile, isDeleted);

        string pkValue = "";
        for (int i = 0; i < meta.totalCols; i++) {
            DataType type = meta.colList[i].dType;
            bool isPK = meta.colList[i].isPrimaryKey;

            if (type == INT) {
                int val; readBinary(dataFile, val);
                if (isPK) pkValue = to_string(val);
            } else if (type == FLOAT) {
                float val; readBinary(dataFile, val);
                if (isPK) pkValue = to_string(val);
            } else if (type == BOOL) {
                bool val; readBinary(dataFile, val);
                if (isPK) pkValue = val ? "true" : "false";
            } else if (type == STRING) {
                vector<char> buffer(meta.colList[i].sizeBytes);
                dataFile.read(buffer.data(), meta.colList[i].sizeBytes);
                if (isPK) pkValue = string(buffer.data());
            }
        }

        if (!isDeleted && pkValue != "") {
            pkIndex[pkValue].push_back({timestamp, offset});
            meta.totalRows++;
        }
    }
}

Table::Table(const TableStruct& tableMeta) {
    meta = tableMeta;
    recoverState(); // rebuild memory on boot!
}

bool Table::insertRecord(const DataRow& row) {
    if (!validateRow(row)) return false; 
    
    string pk = getPK(row);
    if (pkIndex.count(pk)) return false; 

    // appending to _data.dat so we don't mess up the .db schema file
    string fileName = "data/" + string(meta.tblName) + "_data.dat";
    ofstream dataFile(fileName, ios::binary | ios::app);
    if (!dataFile) return false;

    uint64_t offset = dataFile.tellp();
    uint64_t timestamp = chrono::duration_cast<chrono::milliseconds>(
        chrono::system_clock::now().time_since_epoch()
    ).count();
    bool isDeleted = false;

    writeBinary(dataFile, timestamp);
    writeBinary(dataFile, isDeleted);

    for (int i = 0; i < meta.totalCols; i++) {
        DataType type = meta.colList[i].dType;
        const string& val = row.values[i];

        if (type == INT) writeBinary(dataFile, stoi(val));
        else if (type == FLOAT) writeBinary(dataFile, stof(val));
        else if (type == BOOL) writeBinary(dataFile, val == "true");
        else if (type == STRING) writeFixedString(dataFile, val, meta.colList[i].sizeBytes);
    }

    dataFile.close();

    // update ram index
    pkIndex[pk].push_back({timestamp, offset});
    meta.totalRows++; 

    return true;
}

int Table::getRowCount() const { 
    return meta.totalRows; 
}