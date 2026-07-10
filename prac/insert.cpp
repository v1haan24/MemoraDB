#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>
#include <chrono>
#include <cstring>
#include <cassert>

using namespace std;

// keeping same sizes as the catalog file so things don't break
#define cns 30 
#define tns 30 

enum DataType { INT, FLOAT, STRING, BOOL };

// mapping exactly to the catalog structs
struct ColumnInfo {
    char colName[cns];
    bool isPrimaryKey;
    DataType dType;
    int sizeBytes;
    int offsetValue = 0;

    ColumnInfo() {} 
    ColumnInfo(string n, DataType t, bool pk, int s = 0) {
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
    vector<ColumnInfo> colList;
    int totalRows = 0;
    int totalCols = 0;
    char tblName[tns];
    int recordSize = 0;
};

// struct just to hold the incoming row data from parser
struct DataRow {
    vector<string> values;
};

// keeping track of timestamps and byte offsets for the ram index
struct RowVersion {
    uint64_t timestamp;
    uint64_t offset;
};

// standard binary write/read using reinterpret_cast
template<typename T>
void writeBinary(ofstream& file, const T& value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template<typename T>
void readBinary(ifstream& file, T& value) {
    file.read(reinterpret_cast<char*>(&value), sizeof(T));
}

// gotta validate inputs before stoi/stof otherwise the whole program crashes on bad data
bool isIntStr(const string& s) {
    if (s.empty()) return false;
    size_t start = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    if (start == s.length()) return false;
    for (size_t i = start; i < s.length(); i++) {
        if (!isdigit(s[i])) return false;
    }
    return true;
}

bool isFloatStr(const string& s) {
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

class StorageEngine {
    TableStruct meta;
    unordered_map<string, vector<RowVersion>> pkIndex; // our main ram index for quick lookups

    string getPK(const DataRow& row) {
        for (int i = 0; i < meta.totalCols; i++) {
            if (meta.colList[i].isPrimaryKey) return row.values[i];
        }
        return "";
    }

    // making sure the row actually matches our table schema before we do anything
    bool validateRow(const DataRow& row) {
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

    // padding strings so they take up the exact right amount of bytes in the file
    void writeFixedString(ofstream& file, const string& val, int sizeBytes) {
        vector<char> buffer(sizeBytes, '\0');
        memcpy(buffer.data(), val.data(), min((int)val.size(), sizeBytes - 1));
        file.write(buffer.data(), sizeBytes);
    }

    // this is crucial! if the server restarts, we need to read the data file 
    // to rebuild our pkIndex and row count so we don't start from zero
    void recoverState() {
        string fileName = string(meta.tblName) + "_data.dat";
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

                // reading the actual data to find the pk value
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

            // only add it back to ram if it hasn't been deleted
            if (!isDeleted && pkValue != "") {
                pkIndex[pkValue].push_back({timestamp, offset});
                meta.totalRows++;
            }
        }
    }

public:
    StorageEngine(const TableStruct& tableMeta) {
        meta = tableMeta;
        recoverState(); // rebuild memory on boot
    }

    bool insertRecord(const DataRow& row) {
        if (!validateRow(row)) return false; // reject bad inputs early
        
        string pk = getPK(row);
        if (pkIndex.count(pk)) return false; // enforce unique pk

        // appending to _data.dat so we don't mess up the .db schema file
        string fileName = string(meta.tblName) + "_data.dat";
        ofstream dataFile(fileName, ios::binary | ios::app);
        if (!dataFile) return false;

        uint64_t offset = dataFile.tellp();
        uint64_t timestamp = chrono::duration_cast<chrono::milliseconds>(
            chrono::system_clock::now().time_since_epoch()
        ).count();
        bool isDeleted = false;

        writeBinary(dataFile, timestamp);
        writeBinary(dataFile, isDeleted);

        // write the payload based on the column type
        for (int i = 0; i < meta.totalCols; i++) {
            DataType type = meta.colList[i].dType;
            const string& val = row.values[i];

            if (type == INT) writeBinary(dataFile, stoi(val));
            else if (type == FLOAT) writeBinary(dataFile, stof(val));
            else if (type == BOOL) writeBinary(dataFile, val == "true");
            else if (type == STRING) writeFixedString(dataFile, val, meta.colList[i].sizeBytes);
        }

        dataFile.close();

        // finally update ram and row count
        pkIndex[pk].push_back({timestamp, offset});
        meta.totalRows++; 

        return true;
    }

    int getRowCount() const { return meta.totalRows; }
};

int main() {
    // mock table setup for testing
    TableStruct emp;
    strcpy(emp.tblName, "Employee");
    emp.colList.push_back(ColumnInfo("empId", INT, true));
    emp.colList.push_back(ColumnInfo("name", STRING, false, 30));
    emp.colList.push_back(ColumnInfo("salary", FLOAT, false));
    emp.colList.push_back(ColumnInfo("isActive", BOOL, false));
    emp.totalCols = 4;

    StorageEngine engine(emp);

    DataRow validRow{{"101", "VIVAAN", "95000.0", "true"}};
    DataRow crashRow{{"abc", "Vihaan", "xyz", "true"}}; 

    // using asserts to quickly prove our checks work-awwab
    assert(engine.insertRecord(crashRow) == false); 
    assert(engine.insertRecord(validRow) == true);

    cout << "Tests passed. Storage engine running lessssssssssgooooo!\n";

    return 0;
}