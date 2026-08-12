#include "vecTable.h"
#include <iostream>

vecTable::vecTable(const VectorMeta& metadata) : meta(metadata) {}

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
    writeBinary(file, meta.recordCount); //recordcount will act as an int id for each record
    
    std::string padded(meta.pkSize, '\0');
    std::memcpy(&padded[0], pk.data(), pk.size());
    file.write(padded.data(), meta.pkSize);
    writeBinary(file, timestamp);
    file.write(reinterpret_cast<const char*>(embed), sizeof(float)*VEC_DIM);

    if(!file){ std::cerr<<"ERROR: Failed to write record to '"<<meta.tablePath<<"'.\n"; return false; }

    meta.recordCount++;
    file.seekp(sizeof(int)+tns+sizeof(int), std::ios::beg);
    writeBinary(file,meta.recordCount);

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