#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <unordered_set>
#include <cstring>

using namespace std;

#define cns 30 
#define tns 30 

enum DataType { INT, FLOAT, STRING, BOOL };

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

// basic templates for binary read/write
template<typename T>
void writeBin(ofstream& f, const T& val) {
    f.write(reinterpret_cast<const char*>(&val), sizeof(T));
}

template<typename T>
void readBin(ifstream& f, T& val) {
    f.read(reinterpret_cast<char*>(&val), sizeof(T));
}

void writeColInfo(ofstream& file, const ColumnInfo& c) {
    file.write(c.colName, cns);
    writeBin(file, c.dType);
    writeBin(file, c.sizeBytes);
    writeBin(file, c.offsetValue);
    writeBin(file, c.isPrimaryKey);
}

void readColInfo(ifstream& file, ColumnInfo& c) {
    file.read(c.colName, cns);
    readBin(file, c.dType);
    readBin(file, c.sizeBytes);
    readBin(file, c.offsetValue);
    readBin(file, c.isPrimaryKey);
}

// checks if string has a space
bool hasSpace(const char* str) {
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] == ' ') return true;
    }
    return false;
}

class DatabaseCatalog {
    vector<TableStruct> allTables;

    // checks if table already exists
    bool checkExists(string name) {
        for (int i = 0; i < allTables.size(); i++) {
            if (strcmp(allTables[i].tblName, name.c_str()) == 0) return true;
        }
        return false;
    }

    void computeOffsets(TableStruct& t) {
        int currentSize = 0;
        for (int i = 0; i < t.colList.size(); i++) {
            t.colList[i].offsetValue = currentSize;
            currentSize += t.colList[i].sizeBytes;
        }
        t.recordSize = currentSize;
    }

public:
    bool makeNewTable(TableStruct& t) {
        if (strlen(t.tblName) >= tns) return false;
        if (t.colList.size() == 0) return false;
        if (checkExists(t.tblName)) return false;

        // check spaces in table name
        if (hasSpace(t.tblName)) {
            cout << "Error: Table name cannot contain spaces.\n";
            return false;
        }

        unordered_set<string> nameChecker;
        int pkCounter = 0;

        for (int i = 0; i < t.colList.size(); i++) {
            ColumnInfo& c = t.colList[i];
            
            if (strlen(c.colName) >= cns) return false;
            if (hasSpace(c.colName)) {
                cout << "Error: Column name cannot contain spaces.\n";
                return false;
            }
            if (nameChecker.insert(c.colName).second == false) {
                cout << "Error: Duplicate column.\n";
                return false;
            }
            if (c.dType == STRING && c.sizeBytes <= 0) return false;
            if (c.isPrimaryKey && c.dType == BOOL) {
                cout << "Error: Boolean cannot be PK.\n";
                return false;
            }
            if (c.isPrimaryKey) pkCounter++;
        }

        if (pkCounter != 1) return false;

        computeOffsets(t);
        if (t.recordSize == 0) return false;

        string fileName = string(t.tblName) + ".db";
        ofstream file(fileName, ios::binary);
        if (!file) return false;

        t.totalCols = t.colList.size();
        t.metaSize = 46 + 43 * t.totalCols; 

        writeBin(file, t.metaSize);
        file.write(t.tblName, tns);
        writeBin(file, t.totalRows);
        writeBin(file, t.recordSize);
        writeBin(file, t.totalCols);

        for (int i = 0; i < t.totalCols; i++) {
            writeColInfo(file, t.colList[i]);
        }

        allTables.push_back(t);
        file.close();
        return true;
    }

    TableStruct* fetchTable(const string& tblName) {
        for (int i = 0; i < allTables.size(); i++) {
            if (strcmp(allTables[i].tblName, tblName.c_str()) == 0) {
                return &allTables[i];
            }
        }
        return NULL;
    }

    TableStruct loadTable(string fileName) {
        ifstream file(fileName, ios::binary);
        TableStruct t;
        
        if (!file) {
            cout << "Error: Could not open file.\n";
            return t; 
        }

        readBin(file, t.metaSize);
        file.read(t.tblName, tns);
        readBin(file, t.totalRows);
        readBin(file, t.recordSize);
        readBin(file, t.totalCols);

        for (int i = 0; i < t.totalCols; i++) {
            ColumnInfo c;
            readColInfo(file, c);
            t.colList.push_back(c);
        }
        
        file.close();
        return t;
    }
};

int main() {
    DatabaseCatalog db;

    TableStruct emp;
    strcpy(emp.tblName, "Employee");
    emp.colList.push_back(ColumnInfo("empId", INT, true));
    emp.colList.push_back(ColumnInfo("name", STRING, false, 30));
    emp.colList.push_back(ColumnInfo("salary", FLOAT, false));
    emp.colList.push_back(ColumnInfo("isActive", BOOL, false));
    db.makeNewTable(emp);

    TableStruct student;
    strcpy(student.tblName, "Student");
    student.colList.push_back(ColumnInfo("rollNo", INT, true));
    student.colList.push_back(ColumnInfo("studentName", STRING, false, 30));
    student.colList.push_back(ColumnInfo("cgpa", FLOAT, false));
    student.colList.push_back(ColumnInfo("isHosteller", BOOL, false));
    db.makeNewTable(student);

    TableStruct product;
    strcpy(product.tblName, "Product");
    product.colList.push_back(ColumnInfo("productId", INT, true));
    product.colList.push_back(ColumnInfo("productName", STRING, false, 40));
    product.colList.push_back(ColumnInfo("price", FLOAT, false));
    product.colList.push_back(ColumnInfo("inStock", BOOL, false));
    db.makeNewTable(product); 
    
    cout << "Valid tables saved successfully.\n\n";

    TableStruct e = db.loadTable("Employee.db");
    cout << e.tblName << " Row Size: " << e.recordSize << "\n";

    TableStruct s = db.loadTable("Student.db");
    cout << s.tblName << " Row Size: " << s.recordSize << "\n";

    TableStruct p = db.loadTable("Product.db");
    cout << p.tblName << " Row Size : " << p.recordSize << "\n";

    return 0;
}