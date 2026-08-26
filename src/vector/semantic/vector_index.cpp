#include "vector_index.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <queue>
#include <fstream>

std::string VectorIndex::makeKey(const std::string& pk,uint64_t timestamp) const{
    return pk+"#"+std::to_string(timestamp);
}

void VectorIndex::buildIndex(vecTable& vt) {
    table=&vt;
    VectorMeta& meta=vt.getMeta();
    std::ifstream file(meta.tablePath, std::ios::binary);
    if(!file) {std::cerr << "ERROR: Couldn't open vector file.\n"; return;}
    index.clear();
    file.seekg(meta.metadataSize, std::ios::beg);
    for(int i=0;i<meta.recordCount;i++){
        uint32_t id;
        readBinary(file,id);
        std::string padded(meta.pkSize,'\0');
        file.read(&padded[0],meta.pkSize);
        std::string pk(padded.c_str(),strnlen(padded.c_str(), meta.pkSize));
        uint64_t timestamp;
        readBinary(file,timestamp);
        if(!file) {std::cerr<<"ERROR: Failed to read vector index data.\n";return;}
        index[makeKey(pk, timestamp)] = id;
        file.seekg(VEC_DIM * sizeof(float),std::ios::cur);
    }
}

uint32_t VectorIndex::findId(const std::string& pk,uint64_t timestamp) const {
    std::string key=makeKey(pk,timestamp);
    auto it=index.find(key);
    if(it==index.end()) return UINT32_MAX;
    return it->second;
}

float cosineSimilarity(const float (&a)[VEC_DIM],const float (&b)[VEC_DIM]){
    float dot=0.0f;
    float magA=0.0f;
    float magB=0.0f;
    for(int i=0;i<VEC_DIM;i++){
        dot+=a[i]*b[i];
        magA+=a[i]*a[i];
        magB+=b[i]*b[i];
    }

    if(magA==0.0f || magB==0.0f) return 0.0f;
    return dot/(std::sqrt(magA)*std::sqrt(magB));
}

std::vector<SearchResult> VectorIndex::semanticSearch(const std::vector<VCandidate>& candidates,const float (&queryEmbedding)[VEC_DIM],int k){
    if(k<=0) return {};
    auto compare=[](const SearchResult& a,const SearchResult& b){
        return a.score>b.score;
    };

    std::priority_queue<SearchResult,std::vector<SearchResult>,decltype(compare)> heap(compare);
    for(const auto& candidate:candidates){
        uint32_t id=findId(candidate.pk,candidate.timestamp);
        if(id==UINT32_MAX) continue;
        VecRecord record=table->readRecord(id);
        float score=cosineSimilarity(queryEmbedding,record.embedding);
        SearchResult result{candidate.pk,candidate.timestamp,score};
        if(heap.size()<k){
            heap.push(result);
        }
        else if(score>heap.top().score){
            heap.pop();
            heap.push(result);
        }
    }
    std::vector<SearchResult> results;
    while(!heap.empty()) {
        results.push_back(heap.top());
        heap.pop();
    }
    std::reverse(results.begin(), results.end());
    return results;
}
