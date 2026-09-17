#pragma once
#include "../storage/table.h"
#include "vector_meta.h"
#include <unordered_map>

class vecTable{
    private:
    VectorMeta meta;
    std::unordered_map<std::string, uint32_t> index;
    std::string makeKey(const std::string& pk,uint64_t timestamp) const;
    public:
    vecTable(const VectorMeta& metadata);
    bool insert(const std::string& pk, uint64_t timestamp, const float* embed);
    VecRecord readRecord(uint32_t id);

    VectorMeta& getMeta(){ return meta; }
    uint32_t findId(const std::string& pk, uint64_t timestamp) const;
    float cosineSimilarity(const float (&a)[VEC_DIM],const float (&b)[VEC_DIM]);
    std::vector<SearchResult> semanticSearch(const std::vector<VCandidate>& candidates,const float (&queryEmbedding)[VEC_DIM],int k);
    void buildIndex();
    std::vector<VCandidate> refineCandidates(const std::vector<Record>& records,const TableMeta& meta);
};