#include<iostream>
#include<stdio.h> 
#include<vector>
#include<fstream>
#include<algorithm>
#include<cstring>
#include<unordered_set>
#include <iomanip> 
using namespace std;

#define col_name_sz 30
#define table_name_sz 30

enum Datatype {INT, FLOAT, STRING, BOOL};

struct colMeta
{
    char name[col_name_sz];
    Datatype type;
    bool isPK;
    int colSize;
    int offset = 0;

    colMeta()=default;
    colMeta(string n, Datatype t,bool pk,int s=0)
    {
        strncpy(name,n.c_str(),col_name_sz-1);
        name[col_name_sz-1]='\0';
        type=t;isPK=pk;
        if(type==STRING) colSize = s;
        else if(type==INT) colSize = sizeof(int);
        else if(type==FLOAT) colSize = sizeof(float);
        else if(type==BOOL) colSize = sizeof(bool);
    }
};

struct tableMeta
{
    char name[table_name_sz];
    vector<colMeta> cols; //col count can be given by col.size()
    int rowSize = 0;
    int colCount = 0;
    int rowCount = 0; 
    int metadataSize;
};

template<typename T>
void writeBinary(ofstream& file,const T& value){
    file.write(reinterpret_cast<const char*>(&value),sizeof(T));
}

template<typename T>
void readBinary(ifstream& file,T& value){
    file.read(reinterpret_cast<char*>(&value),sizeof(T));
}

void writeColumn(ofstream& file,const colMeta& col){
    file.write(col.name,col_name_sz);
    writeBinary(file,col.type);
    writeBinary(file,col.colSize);
    writeBinary(file,col.offset);
    writeBinary(file,col.isPK);
}

void readColumn(ifstream& file,colMeta& col){
    file.read(col.name,col_name_sz);
    readBinary(file,col.type);
    readBinary(file,col.colSize);
    readBinary(file,col.offset);
    readBinary(file,col.isPK);
}

class Catalog
{
    private:
    vector<tableMeta> tables;
    bool exist(const string& tableName)
    {
        for(auto &t:tables) if(strcmp(t.name,tableName.c_str())==0) return true;
        return false;
    }

    void CalcOffset(tableMeta& table)
    {
        int size=0;
        for(auto &c:table.cols){
            c.offset=size;
            size+=c.colSize;
        }
        table.rowSize=size;
    }

    public:

    bool createTable(tableMeta& table)
    {
        if(table.cols.empty()) return false;
        unordered_set<string> names;
        int pkCount=0;
        for(const auto& col:table.cols)
        {
            if(!names.insert(col.name).second) return false;
            if(col.type==STRING && col.colSize<=0) return false;
            if(col.isPK) pkCount++;
        }
        if(pkCount!=1) return false;
        if(exist(table.name)) return false;

        CalcOffset(table);
        if(table.rowSize==0) return false;

        ofstream file((string)table.name + ".db", ios::binary);
        if(!file) return false;

        table.colCount = table.cols.size();

        table.metadataSize = 4 + sizeof(table.name) + sizeof(table.rowSize) + sizeof(table.colCount) + sizeof(table.rowCount) + 43*table.cols.size();

        writeBinary(file,table.metadataSize);

        file.write(table.name,table_name_sz);
        writeBinary(file,table.rowCount);
        writeBinary(file,table.rowSize);

        writeBinary(file,table.colCount);
        for(const auto& col : table.cols) writeColumn(file,col);
        
        tables.push_back(table);
        return true;
    }

    tableMeta* getTable(const string& tableName)
    {
        for(auto &t:tables)
        {
            if(strcmp(t.name,tableName.c_str())==0) return &t;
       }
       return NULL;
    }

    tableMeta readMetadata(string fileName)
    {
        ifstream file(fileName,ios::binary);
        if(!file) return {};

        tableMeta temp;

        readBinary(file,temp.metadataSize);
        file.read(temp.name,table_name_sz);
        readBinary(file,temp.rowCount);
        readBinary(file,temp.rowSize);

        readBinary(file,temp.colCount);
        for(int i=0;i<temp.colCount;i++)
        {
            colMeta col;
            readColumn(file,col);
            temp.cols.push_back(col);
        }

        return temp;

    }

    
};



int main() {
    Catalog catalog;

    // 1. Create a table schema for "Users"
    tableMeta userTable;
    strncpy(userTable.name, "Users", table_name_sz - 1);
    userTable.name[table_name_sz - 1] = '\0';

    // 2. Define columns: name, datatype, isPrimaryKey, [string length]
    userTable.cols.push_back(colMeta("ID", INT, true));
    userTable.cols.push_back(colMeta("Username", STRING, false, 20));
    userTable.cols.push_back(colMeta("Age", INT, false));
    userTable.cols.push_back(colMeta("IsActive", BOOL, false));

    cout << "=== Creating Table 'Users' ===" << endl;
    if (catalog.createTable(userTable)) {
        cout << "Table 'Users' successfully created and saved to disk!" << endl;
    } else {
        cerr << "Failed to create table! Check constraints (PK count, duplicate names, etc.)." << endl;
        return 1;
    }

    // 3. Read the metadata back from the generated binary file
    cout << "Reading metadata back from 'Students.db' to verify correctness..." << endl;
    
    tableMeta readTable = catalog.readMetadata("Users.db");

    // 4. Display and cross-verify the results
    string typeLookup[] = {"INT", "FLOAT", "STRING", "BOOL"};
    
    cout << "\n================ TABLE METADATA ================" << endl;
    cout << "Table Name  : " << readTable.name << endl;
    cout << "Header Size : " << readTable.metadataSize << " bytes" << endl;
    cout << "Row Size    : " << readTable.rowSize << " bytes" << endl;
    cout << "Row Count   : " << readTable.rowCount << endl;
    cout << "Column Count: " << readTable.colCount << endl;
    cout << "================================================" << endl;
    
    cout << "Columns Layout:" << endl;
    for (const auto &c : readTable.cols)
    {
        cout <<left
             << " -> Name: " << setw(20) << c.name 
             << " | Type: " << setw(10) << typeLookup[c.type]
             << " | Size: " << setw(4) << c.colSize << " bytes"
             << " | Offset: " << setw(4) << c.offset 
             << " | Is PK: " << setw(6) << (c.isPK ? "Yes" : "No") 
             << endl;
    }
    cout << "================================================" << endl;

    return 0;
}
