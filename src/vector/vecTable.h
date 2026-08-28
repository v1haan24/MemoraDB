#pragma once
#include "../storage/table.h"
#include "vector_meta.h"
#include <unordered_set>

class vecTable{
    private:
    //file
    std::fstream file;
    VectorMeta meta;
    bool rewriteKeeping(const std::unordered_set<uint32_t>& keep, const std::string& archiveTag);

    public:
    vecTable(const VectorMeta& metadata);
    bool insert(const std::string& pk, uint64_t timestamp, const float* embed);
    VecRecord readRecord(uint32_t id);
    bool compact(uint64_t timestamp); 
    bool purge(const std::vector<std::string>& pks, uint64_t timestamp);

    VectorMeta& getMeta(){ return meta; }
};