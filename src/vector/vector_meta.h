#pragma once
#include <vector>
#include <string>
#include <cstring>
#include "../common/constants.h"
#include "../storage/table.h"
#include <filesystem>
#include <cstdint>

struct VectorMeta{
    uint32_t metadataSize;
    char name[tns];
    uint32_t pkSize=0;
    uint32_t recordCount=0;
    uint32_t payloadSize=0;
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
    VectorMeta readMetadata(const std::string& fileName);
};