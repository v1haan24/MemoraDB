#pragma once
#include <vector>
#include <string>
#include <cstring>
#include "../common/constants.h"
#include "../storage/table.h"
#include <filesystem>

struct VectorMeta{
    int metadataSize;
    uint32_t recordCount=0;
    char name[tns];
    int pkSize=0;
    int payloadSize=0;
    std::filesystem::path tablePath;
};

struct VecRecord{
    uint32_t id=0;
    std::string pk;
    uint64_t timestamp=0;
    float embedding[VEC_DIM]={};
};

class vecMeta{
    public:
    bool createVecTable(VectorMeta& vec, TableMeta& table);
    VectorMeta readMetadata(const std::string& fileName, TableMeta& table); 
};