#include<iostream>
#include<stdio.h> 
#include<vector>
#include<fstream>
#include<algorithm>
#include<cstring> 
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
};

struct tableMeta
{
    char name[table_name_sz];
    vector<colMeta> cols; //col count can be given by col.size()
    int rowSize = 0;
    int colCount = 0;
    //int rowCount = 0; TO BE DONE
};

class Catalog
{
    private:
    vector<tableMeta> tables;

    public:

    bool createTable(tableMeta& table)
    {
        for(auto &t:tables)
        {
            if(t.name==table.name) return false;
        }

        if(table.rowSize==0)
        {
            int size=0;
            for(auto &c:table.cols)
            {
                c.offset=size;
                size+=c.colSize;
            }
            table.rowSize=size;
        }

        ofstream file((string)table.name + ".db", ios::binary);

        int y = 4 + 43*table.cols.size() + sizeof(table.name) + sizeof(table.rowSize) + sizeof(table.colCount);
        file.write(reinterpret_cast<const char*>(&y), sizeof(y));

        file.write(table.name, sizeof(table.name));
        file.write(reinterpret_cast<const char*>(&table.rowSize), sizeof(table.rowSize));
        table.colCount = table.cols.size();
        file.write(reinterpret_cast<const char*>(&table.colCount),sizeof(table.colCount));

        for(const auto &c : table.cols)
        {
            file.write(c.name, sizeof(c.name));
            file.write(reinterpret_cast<const char*>(&c.type),sizeof(c.type));
            file.write(reinterpret_cast<const char*>(&c.colSize),sizeof(c.colSize));
            file.write(reinterpret_cast<const char*>(&c.offset),sizeof(c.offset));
            file.write(reinterpret_cast<const char*>(&c.isPK),sizeof(c.isPK));
        }

        file.close();
        tables.push_back(table);
        return true;
    }

    tableMeta* getTable(const string& tableName)
    {
        for(auto &t:tables)
        {
            if(t.name==tableName) return &t;
        }
        return NULL;
    }

    tableMeta readMetadata(string fileName, int& header_sz)
    {
        ifstream file(fileName,ios::binary);
        tableMeta temp;

        file.read(reinterpret_cast<char*>(&header_sz), sizeof(header_sz));

        file.read(temp.name, sizeof(temp.name));
        file.read(reinterpret_cast<char*>(&temp.rowSize), sizeof(temp.rowSize));
        file.read(reinterpret_cast<char*>(&temp.colCount),sizeof(temp.colCount));

        for(int i = 0; i<temp.colCount; i++)
        {
            colMeta c;

            file.read(c.name, sizeof(c.name));
            file.read(reinterpret_cast<char*>(&c.type),sizeof(c.type));
            file.read(reinterpret_cast<char*>(&c.colSize),sizeof(c.colSize));
            file.read(reinterpret_cast<char*>(&c.offset),sizeof(c.offset));
            file.read(reinterpret_cast<char*>(&c.isPK),sizeof(c.isPK));

            temp.cols.push_back(c);
        }

        file.close();
        return temp;

    }
};

int main()
{
    Catalog catalog;

    // 1. Initialize metadata for a "Students" table
    tableMeta studentTable;
    strcpy(studentTable.name, "Students");

    // Column 1: ID (INT, Primary Key)
    colMeta col1;
    strcpy(col1.name, "ID");
    col1.type = INT;
    col1.colSize = sizeof(int);
    col1.isPK = true;
    studentTable.cols.push_back(col1);

    // Column 2: Name (STRING)
    colMeta col2;
    strcpy(col2.name, "Name");
    col2.type = STRING;
    col2.colSize = 20; // Max 20 characters allocated
    col2.isPK = false;
    studentTable.cols.push_back(col2);

    // Column 3: GPA (FLOAT)
    colMeta col3;
    strcpy(col3.name, "GPA");
    col3.type = FLOAT;
    col3.colSize = sizeof(float);
    col3.isPK = false;
    studentTable.cols.push_back(col3);

    // 2. Register the table schema (Writes "Students.db")
    cout << "Creating table 'Students' and writing metadata to disk..." << endl;
    //catalog.createTable(studentTable);
    if(catalog.createTable(studentTable))
        cout << "Table created successfully!\n\n";
    else
        cout << "Table already exists!\n\n";

    // 3. Read the metadata back from the generated binary file
    cout << "Reading metadata back from 'Students.db' to verify correctness..." << endl;
    int verifiedHeaderSz = 0;
    tableMeta readTable = catalog.readMetadata("Students.db", verifiedHeaderSz);

    // 4. Display and cross-verify the results
    string typeLookup[] = {"INT", "FLOAT", "STRING", "BOOL"};
    
    cout << "\n================ TABLE METADATA ================" << endl;
    cout << "Table Name  : " << readTable.name << endl;
    cout << "Header Size : " << verifiedHeaderSz << " bytes" << endl;
    cout << "Row Size    : " << readTable.rowSize << " bytes" << endl;
    cout << "Column Count: " << readTable.colCount << endl;
    cout << "================================================" << endl;
    
    cout << "Columns Layout:" << endl;
    for (const auto &c : readTable.cols)
    {
        cout << " -> Name: " << c.name 
             << " \t| Type: " << typeLookup[c.type]
             << " \t| Size: " << c.colSize << " bytes"
             << " \t| Offset: " << c.offset 
             << " \t| Is PK: " << (c.isPK ? "Yes" : "No") 
             << endl;
    }
    cout << "================================================" << endl;

    return 0;
}