#include "vecTable.h"
#include <iostream>
#include <cmath>
#include<algorithm>
#include <queue>

void vecTable::buildIndex() {
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

vecTable::vecTable(const VectorMeta& metadata) : meta(metadata) {}
std::string vecTable::makeKey(const std::string& pk,uint64_t timestamp) const{
    return pk+"#"+std::to_string(timestamp);
}
uint32_t vecTable::findId(const std::string& pk,uint64_t timestamp) const {
    std::string key=makeKey(pk,timestamp);
    auto it=index.find(key);
    if(it==index.end()) return UINT32_MAX;
    return it->second;
}

float vecTable::cosineSimilarity(const float (&a)[VEC_DIM],const float (&b)[VEC_DIM]){
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

std::vector<SearchResult> vecTable::semanticSearch(const std::vector<VCandidate>& candidates,const float (&queryEmbedding)[VEC_DIM],int k){
    if(k<=0) return {};
    auto compare=[](const SearchResult& a,const SearchResult& b){
        return a.score>b.score;
    };

    std::priority_queue<SearchResult,std::vector<SearchResult>,decltype(compare)> heap(compare);
    for(const auto& candidate:candidates){
        uint32_t id=findId(candidate.pk,candidate.timestamp);
        if(id==UINT32_MAX) continue;
        VecRecord record=readRecord(id);
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

std::vector<VCandidate> vecTable::refineCandidates(const std::vector<Record>& records,const TableMeta& meta){
    std::vector<VCandidate> candidates;
    candidates.reserve(records.size());
    int pkIndex=-1;
    for(int i=0;i<meta.columnCount;i++){
        if(meta.columns[i].isPK){
            pkIndex=i;
            break;
        }
    }
    if(pkIndex==-1) return candidates;
    for(const auto& record:records){
        if(record.row.values.empty()) continue;
        if(record.deleted) continue;
        candidates.push_back({record.row.values[pkIndex],record.timestamp});
    }
    return candidates;
}


bool vecTable::insert(const std::string& pk, uint64_t timestamp, const float* embed){
    if(pk.empty()){ 
        std::cerr<<"ERROR: primary key cannot be empty.\n"; 
        return false; 
    }
    if((int)pk.size() > meta.pkSize){
        std::cerr<<"ERROR: primary key '"<<pk<<"' exceeds fixed size of "<<meta.pkSize<<" bytes.\n";
        return false;
    }

    std::fstream file(meta.tablePath, std::ios::binary|std::ios::in|std::ios::out);
    if(!file){ 
        std::cerr<<"ERROR: Couldn't open .vec file '"<<meta.tablePath<<"'!\n"; 
        return false; 
    }

    file.clear();
    file.seekp(0,std::ios::end);
    uint32_t id = meta.recordCount;
    writeBinary(file, meta.recordCount); //recordcount will act as an int id for each record
    
    std::string padded(meta.pkSize, '\0');
    std::memcpy(&padded[0], pk.data(), pk.size());
    file.write(padded.data(), meta.pkSize);
    writeBinary(file, timestamp);
    file.write(reinterpret_cast<const char*>(embed),sizeof(float)*VEC_DIM);

    if(!file){ std::cerr<<"ERROR: Failed to write record to '"<<meta.tablePath<<"'.\n"; return false; }

    meta.recordCount++;
    file.seekp(sizeof(int)+tns+sizeof(int), std::ios::beg);
    writeBinary(file,meta.recordCount);
    index[makeKey(pk, timestamp)]=id;
    file.close();
    return true;
}

VecRecord vecTable::readRecord(uint32_t id){
    VecRecord rec;
    if(id >= meta.recordCount){
        std::cerr<<"ERROR: record id "<<id<<" out of range (recordCount="<<meta.recordCount<<").\n";
        return rec;
    }
 
    std::ifstream file(meta.tablePath, std::ios::binary);
    if(!file){ std::cerr<<"ERROR: Couldn't open .vec file '"<<meta.tablePath<<"'!\n"; return rec; }
 
    uint64_t offset = (uint64_t)meta.metadataSize + (uint64_t)id*(uint64_t)meta.payloadSize;
    file.seekg(offset, std::ios::beg);
    if(!file){ std::cerr<<"ERROR: failed to seek to record "<<id<<".\n"; return rec; }
 
    readBinary(file, rec.id);
 
    std::string padded(meta.pkSize, '\0');
    file.read(&padded[0], meta.pkSize);
    padded.resize(strnlen(padded.c_str(), meta.pkSize));
    rec.pk = padded;
 
    readBinary(file, rec.timestamp);
    readBinary(file, rec.embedding);
 
    if(!file){ std::cerr<<"ERROR: failed to read record "<<id<<".\n"; return VecRecord{}; }
    return rec;
}