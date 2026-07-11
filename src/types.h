#pragma once
#include<vector>
#include<cstring>
#include<string>
using namespace std;

#define col_name_sz 30 //fixed col name size
#define table_name_sz 30 //fixed table name size
#define row_hdsize (sizeof(uint64_t)+sizeof(bool)) //Fixed Row header size: Timestamp + isDeleted

enum DataType {INT,FLOAT,STRING,BOOL};

struct ColMeta  
{
    char name[col_name_sz];
    DataType type;
    bool isPK;
    int colSize;
    int offset = 0;

    ColMeta()=default;
    ColMeta(string n, DataType t,bool pk,int s=0)
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

struct TableMeta
{
    char name[table_name_sz];
    vector<ColMeta> columns; 
    int payloadSize = 0; //Row size but without the row header
    int colCount = 0;
    int rowCount = 0; 
    int metadataSize;
};

struct RecordVersion  //Will be used for, in-memory hashmap
{
    uint64_t timestamp;
    uint64_t offset;
};