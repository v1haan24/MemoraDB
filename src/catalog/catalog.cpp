#include <iostream>
#include "catalog.h"
#include "../storage/serializer.h"
#include <fstream>
#include <unordered_set>
#include <cstring>
using namespace std;

bool Catalog::exist(const string& tableName)
{
    for(auto &t:tables) if(strcmp(t.name,tableName.c_str())==0) return true;
    return false;
}

void Catalog::CalcOffset(TableMeta& table)
{
    int size=0;
    for(auto &c:table.columns)
    {
        c.offset=size;
        size+=c.colSize;
    }
    table.payloadSize=size;
}

bool Catalog::createTable(TableMeta& table)
{
    //if(strlen(table.name)>=tns) return false; ==> This will not be required as we'll truncate table name before inputting
    if(table.columns.empty())
    {
        cerr<<"WARNING: There are no columns in the table!! Table must have atleast one column"<<endl;
        return false;
    } 
    unordered_set<string> names;
    int pkCount=0;
    for(const auto& col:table.columns)
    {
        //if(strlen(col.name)>=col_name_sz) return false; ==> Not required as we are already truncating column name
        if(!names.insert(col.name).second)
        {
            cerr<<"ERROR: Duplicate column name found!"<<endl;
            return false;
        } 
        if(col.type==STRING && col.colSize<=0)
        {
            cerr<<"ERROR: A string column size must be positive!"<<endl;
            return false;
        } 
        if(col.isPK) pkCount++;
    }

    if(pkCount == 0) 
    {
        cerr<<"ERROR: NO PRIMARY KEY found!"<<endl;
        return false; 
    }
    if(pkCount > 1) //This is done as we are currently considering only single column as primary key
    {
        cerr<<"ERROR: More than one PRIMARY KEY found!"<<endl;
        return false; 
    } 

    if(exist(table.name))
    {
        cerr<<"WARNING: Table already exists!! Duplicate table name!"<<endl;
        return false;
    } 

    CalcOffset(table);
    if(table.payloadSize==0)
    {
        cerr<<"ERROR: Payload size is ZERO!"<<endl;
        return false;
    } 

    ofstream file("data/"+string(table.name)+".db",ios::binary); //Opening file in Binary mode for writing
    if(!file) return false;

    table.colCount = table.columns.size();

    table.metadataSize=
                sizeof(int) + table_name_sz + sizeof(int) + sizeof(int) + sizeof(int) +
                table.colCount*(col_name_sz + sizeof(DataType) + sizeof(int) + sizeof(int) + sizeof(bool)); //<--- if change in cns,tns change this too
    //table.metadataSize = 4 + sizeof(table.name) + sizeof(table.payloadSize) + sizeof(table.colCount) + sizeof(table.rowCount) + 43*table.columns.size();

    writeBinary(file,table.metadataSize);

    file.write(table.name,table_name_sz);
    writeBinary(file,table.rowCount);
    writeBinary(file,table.payloadSize);

    writeBinary(file,table.colCount);
    for(const auto& col : table.columns) writeColumn(file,col);
    
    tables.push_back(table);
    
    return true;
}

TableMeta* Catalog::getTable(const string& tableName)
{
    for(auto &t:tables)
    {
        if(strcmp(t.name,tableName.c_str())==0) return &t;
    }
    return nullptr;
}

TableMeta Catalog::readMetadata(string fileName)
{
    ifstream file("data/"+fileName,ios::binary);
    if(!file)
    {
        cerr<<"ERROR: Couldn't open file!"<<endl;
        return {};
    } 

    TableMeta temp;
    readBinary(file,temp.metadataSize);
    file.read(temp.name,table_name_sz);
    readBinary(file,temp.rowCount);
    readBinary(file,temp.payloadSize);

    readBinary(file,temp.colCount);
    for(int i=0;i<temp.colCount;i++)
    {
        ColMeta col;
        readColumn(file,col);
        temp.columns.push_back(col);
    }
    return temp;
}