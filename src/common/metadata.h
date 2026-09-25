#pragma once
#include <vector>
#include <string>
#include <cstring>
#include "constants.h"
#include <cstring>
#include <cstdint>

enum DataType : int32_t {INT,FLOAT,STRING,BOOL};

struct ColMeta{
    char name[cns];
    uint8_t isPK;
    uint8_t isSemantic=0;
    DataType type;
    uint32_t size;
    uint32_t offset=0;
    ColMeta()=default;
    ColMeta(std::string n,DataType t,bool pk,int32_t s=0){
        strncpy(name,n.c_str(),cns-1);
        name[cns-1]='\0';
        type=t;isPK=pk;
        if(type==STRING) size=s;
        else if(type==INT) size=sizeof(int32_t);
        else if(type==FLOAT) size=sizeof(float);
        else if(type==BOOL) size=sizeof(uint8_t);
    }
};

struct TableMeta{
    uint32_t metadataSize;
    std::vector<ColMeta> columns;
    //int rowCount=0;
    uint32_t columnCount;
    char name[tns];
    uint32_t payloadSize=0;
};

struct RecordVersion{
    uint64_t timestamp;
    uint64_t offset;
};

struct Row{
    std::vector<std::string> values; 
};

struct Record{
    uint64_t timestamp;
    uint8_t deleted;
    Row row;
};

struct Difference{
    uint64_t timestamp=0;
    std::string column;
    std::string before;
    std::string after;
};

struct VCandidate{
    std::string pk;
    uint64_t timestamp;
};

struct SearchResult {
    std::string pk;
    uint64_t timestamp;
    float score;
    std::vector<std::string> semanticValues;
};
