#pragma once
#include <vector>
#include <string>
#include <cstring>
#include "../common/constants.h"
#include "../storage/table.h"
#include <filesystem>

struct VectorMeta{
    int metadataSize;
    uint64_t rowCount=0;
    char name[tns];
    int payloadSize=0;
    std::filesystem::path tablePath;
};

class vecMeta{
    private:
    VectorMeta readMetadata(const std::string& fileName);
    

    public:
    bool createVecTable(VectorMeta& vec, TableMeta& table);
    
};