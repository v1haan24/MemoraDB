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
    bool writeRewriteHeader(std::fstream& out);
    bool finalizeRewrite(std::fstream& out, const std::string& outPath, uint32_t recordCount, const std::string& archiveTag);

    std::fstream tempFile;
    std::string tempPath;
    uint32_t newId = 0;
    bool rewriting = false;

    public:
    vecTable(const VectorMeta& metadata);
    bool insert(const std::string& pk, uint64_t timestamp, const float* embed);
    VecRecord readRecord(uint32_t id);
    bool purge(const std::vector<std::string>& pks, uint64_t timestamp);

    bool startRewrite();
    bool copyRecord(uint32_t oldId);
    bool finishRewrite(const std::string& archiveTag);
    void cancelRewrite();

    VectorMeta& getMeta(){ return meta; }
};