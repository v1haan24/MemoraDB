#include "semantic/vector_index.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdint>

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