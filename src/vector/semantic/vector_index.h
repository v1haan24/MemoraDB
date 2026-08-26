#pragma once
#include "../vecTable.h"
#include <unordered_map>
#include <string>
#include <cstdint>
#include <vector>


class VectorIndex{
    private:
    vecTable* table=nullptr; 
    std::unordered_map<std::string, uint32_t> index;
    std::string makeKey(const std::string& pk,uint64_t timestamp) const;

    public:
    void buildIndex(vecTable& vt);
    uint32_t findId(const std::string& pk, uint64_t timestamp) const;
    std::vector<SearchResult> semanticSearch(const std::vector<VCandidate>& candidates,const float (&queryEmbedding)[VEC_DIM],int k);
};

float cosineSimilarity(const float (&a)[VEC_DIM],const float (&b)[VEC_DIM]);
