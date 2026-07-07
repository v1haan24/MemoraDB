#include<iostream>
#include<stdio.h> 
#include<vector>
#include<fstream>
#include<algorithm>
#include<cstring> 
using namespace std;

enum DataType {INT,FLOAT,STRING,BOOL};

struct ColMeta{
    string name;
    DataType type;
    bool isPK;
    int size;
    int offset=0;
};
struct TableMeta{
    vector<ColMeta> colum;
    string name;
    int rowSize=0;
};

class Catalog
{
private:
        vector<TableMeta> tables;
public:

    bool createTable(TableMeta& table){
       for(auto &t:tables){
        if(t.name==table.name) return false;
       }

       if(table.rowSize==0){
        int size=0;
        for(auto &c:table.colum){
            c.offset=size;
            size+=c.size;
        }
        table.rowSize=size;
       }

       ofstream file(table.name+".db",ios::binary);
       
    int ntsz=table.name.length();
        file.write(reinterpret_cast<const char*>(&ntsz),sizeof(ntsz));
        file.write(table.name.data(),ntsz);
    file.write(reinterpret_cast<const char*>(&table.rowSize),sizeof(table.rowSize));

    int colcnt=table.colum.size();
    file.write(reinterpret_cast<const char*>(&colcnt),sizeof(colcnt));
    
    for(const auto &c:table.colum){
        int nsz=c.name.length();
        file.write(reinterpret_cast<const char*>(&nsz),sizeof(nsz));
        file.write(c.name.data(),nsz);
        file.write(reinterpret_cast<const char*>(&c.type),sizeof(c.type));
        file.write(reinterpret_cast<const char*>(&c.size),sizeof(c.size));
         file.write(reinterpret_cast<const char*>(&c.offset),sizeof(c.offset));
         file.write(reinterpret_cast<const char*>(&c.isPK),sizeof(c.isPK));
    }
       file.close();
       tables.push_back(table);
        return true;
    }


    TableMeta* getTable(const string& tableName){
        for(auto &t:tables){
            if(t.name==tableName) return &t;
       }
       return NULL;
    }

    TableMeta readMetadata(string fileName){
        ifstream file(fileName,ios::binary);
        TableMeta temp;

        int len;
        file.read(reinterpret_cast<char*>(&len),sizeof(len));
        temp.name.resize(len);
        file.read(&temp.name[0],len);

        file.read(reinterpret_cast<char*>(&temp.rowSize),sizeof(temp.rowSize));

        int colcnt;
        file.read(reinterpret_cast<char*>(&colcnt),sizeof(colcnt));
        for(int i=0;i<colcnt;i++){
            //length name type size offset ispk
            ColMeta tem;

            int len;
            file.read(reinterpret_cast<char*>(&len),sizeof(len));
            tem.name.resize(len);
            file.read(&tem.name[0],len);

            file.read(reinterpret_cast<char*>(&tem.type),sizeof(tem.type));
            file.read(reinterpret_cast<char*>(&tem.size),sizeof(tem.size));
            file.read(reinterpret_cast<char*>(&tem.offset),sizeof(tem.offset));
            file.read(reinterpret_cast<char*>(&tem.isPK),sizeof(tem.isPK));
            temp.colum.push_back(tem);
        }
        file.close();
        return temp;
    }
    
};

//testing data generated form gpt
int main()
{
    Catalog catalog;

    // Create table
    TableMeta employee;
    employee.name = "Employee";

    // id
    ColMeta id;
    id.name = "id";
    id.type = INT;
    id.size = 4;
    id.isPK = true;

    // name
    ColMeta name;
    name.name = "name";
    name.type = STRING;
    name.size = 21;      // VARCHAR(20)
    name.isPK = false;

    // salary
    ColMeta salary;
    salary.name = "salary";
    salary.type = FLOAT;
    salary.size = 4;
    salary.isPK = false;

    // active
    ColMeta active;
    active.name = "active";
    active.type = BOOL;
    active.size = 1;
    active.isPK = false;

    employee.colum.push_back(id);
    employee.colum.push_back(name);
    employee.colum.push_back(salary);
    employee.colum.push_back(active);

    cout << "Creating Employee table...\n\n";

    if(catalog.createTable(employee))
        cout << "Table created successfully!\n\n";
    else
        cout << "Table already exists!\n\n";

    cout << "---------- ORIGINAL METADATA ----------\n";

    cout << "Table Name : " << employee.name << endl;
    cout << "Row Size   : " << employee.rowSize << endl;

    for(const auto &c : employee.colum)
    {
        cout << "\nColumn Name : " << c.name << endl;
        cout << "Type        : " << c.type << endl;
        cout << "Size        : " << c.size << endl;
        cout << "Offset      : " << c.offset << endl;
        cout << "Primary Key : " << c.isPK << endl;
    }

    cout << "\n\nReading metadata back from Employee.db...\n\n";

    TableMeta read = catalog.readMetadata("Employee.db");

    cout << "---------- READ METADATA ----------\n";

    cout << "Table Name : " << read.name << endl;
    cout << "Row Size   : " << read.rowSize << endl;

    for(const auto &c : read.colum)
    {
        cout << "\nColumn Name : " << c.name << endl;
        cout << "Type        : " << c.type << endl;
        cout << "Size        : " << c.size << endl;
        cout << "Offset      : " << c.offset << endl;
        cout << "Primary Key : " << c.isPK << endl;
    }

    return 0;
}