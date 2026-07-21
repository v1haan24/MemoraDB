#pragma once
#include <vector>
#include <string>
#include <cstring>
#include "constants.h"
#include <cstring>
#include <cstdint>
using namespace std;

struct ColMeta{
    char name[cns];
    bool isPK;
    DataType type;
    int size;
    int offset=0;
    ColMeta()=default;
    ColMeta(string n,DataType t,bool pk,int s=0){
        strncpy(name,n.c_str(),cns-1);
        name[cns-1]='\0';
        type=t;isPK=pk;
        if(type==STRING) size=s;
        else if(type==INT) size=sizeof(int);
        else if(type==FLOAT) size=sizeof(float);
        else if(type==BOOL) size=sizeof(bool);
    }
};

struct TableMeta{
    int metadataSize;
    vector<ColMeta> columns;
    int rowCount=0;
    int columnCount;
    char name[tns];
    int payloadSize=0;
};

struct RecordVersion{
    uint64_t timestamp;
    uint64_t offset;
};

struct Row{
    vector<string> values; //assuming parser gives string as output
};

struct Record{
    uint64_t timestamp;
    bool deleted;
    Row row;
};