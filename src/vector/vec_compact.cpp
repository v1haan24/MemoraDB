
#include "vecTable.h"
#include <iostream>
#include <cstdio>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>

bool vecTable::rewriteKeeping(const std::unordered_set<uint32_t>& keep, const std::string& archiveTag){
    if(!file.is_open()){
        file.clear();
        file.open(meta.tablePath, std::ios::binary|std::ios::in|std::ios::out);
    }
    if(!file.is_open()){ std::cerr<<"Failed to open vec file.\n"; return false; }

    std::string tempPath=meta.tablePath.string()+".tmp";
    std::fstream out(tempPath, std::ios::binary|std::ios::in|std::ios::out|std::ios::trunc);
    if(!out.is_open()){ std::cerr<<"Failed to create compact vec file.\n"; return false; }

    writeBinary(out, meta.metadataSize);
    out.write(meta.name, tns);
    writeBinary(out, meta.pkSize);
    uint32_t placeholderCount=0;
    writeBinary(out, placeholderCount);
    writeBinary(out, meta.payloadSize);

    file.clear();
    file.seekg(meta.metadataSize, std::ios::beg);
    uint32_t newId=0;
    for(uint32_t i=0;i<meta.recordCount;i++){
        uint32_t id;
        std::string padded(meta.pkSize, '\0');
        uint64_t ts;
        float embed[VEC_DIM];
        readBinary(file, id);
        file.read(&padded[0], meta.pkSize);
        readBinary(file, ts);
        file.read(reinterpret_cast<char*>(embed), sizeof(float)*VEC_DIM);
        if(!file){
            std::cerr<<"Corrupted record encountered while rewriting vec file.\n";
            out.close();
            std::remove(tempPath.c_str());
            return false;
        }
        if(!keep.count(id)) continue;

        writeBinary(out, newId);
        out.write(padded.data(), meta.pkSize);
        writeBinary(out, ts);
        out.write(reinterpret_cast<const char*>(embed), sizeof(float)*VEC_DIM);
        newId++;
    }
    if(!out){
        std::cerr<<"Failed to write rewritten vec file.\n";
        out.close();
        std::remove(tempPath.c_str());
        return false;
    }

    out.seekp(sizeof(int)+tns+sizeof(int), std::ios::beg);
    writeBinary(out, newId);
    out.close();

    file.close();

    std::string tableName=meta.tablePath.parent_path().filename().string();
    std::string archivePath="data/"+tableName+"/archive/archive_vec_"+archiveTag+".vec";
    std::filesystem::create_directories("data/"+tableName+"/archive");

    if(std::filesystem::exists(archivePath)){
        std::cerr<<"Vector archive already exists.\n";
        std::remove(tempPath.c_str());
        file.open(meta.tablePath, std::ios::binary|std::ios::in|std::ios::out);
        return false;
    }
    if(std::rename(meta.tablePath.string().c_str(), archivePath.c_str())!=0){
        std::cerr<<"Failed to archive old vector file.\n";
        file.open(meta.tablePath, std::ios::binary|std::ios::in|std::ios::out);
        return false;
    }
    if(std::rename(tempPath.c_str(), meta.tablePath.string().c_str())!=0){
        std::cerr<<"Failed to activate rewritten vector file.\n";
        std::rename(archivePath.c_str(), meta.tablePath.string().c_str());
        file.open(meta.tablePath, std::ios::binary|std::ios::in|std::ios::out);
        return false;
    }

    meta.recordCount=newId;
    file.open(meta.tablePath, std::ios::binary|std::ios::in|std::ios::out);
    if(!file.is_open()){
        std::cerr<<"ERROR: failed to reopen vec file after rewrite.\n";
        return false;
    }
    return true;
}

bool vecTable::compact(uint64_t timestamp){

    if(!file.is_open()){
        file.clear();
        file.open(meta.tablePath, std::ios::binary|std::ios::in|std::ios::out);
    }
    if(!file.is_open()){ std::cerr<<"Failed to open vec file for compaction.\n"; return false; }
    std::unordered_map<std::string, std::vector<VecRecord>> versions;
    file.clear();
    file.seekg(meta.metadataSize, std::ios::beg);
    for(uint32_t i=0;i<meta.recordCount;i++){
        VecRecord rec;
        readBinary(file, rec.id);
        std::string padded(meta.pkSize, '\0');
        file.read(&padded[0], meta.pkSize);
        readBinary(file, rec.timestamp);
        file.read(reinterpret_cast<char*>(rec.embedding), sizeof(float)*VEC_DIM);
        if(!file){
            std::cerr<<"Corrupted record during vec compaction.\n";
            return false;
        }
        rec.pk.assign(padded.c_str(), strnlen(padded.c_str(), meta.pkSize));
        versions[rec.pk].push_back(std::move(rec));
    }

    std::unordered_set<uint32_t> keep;
    for(const auto& [pk, recs] : versions){
        const VecRecord* anchor=nullptr;
        for(auto it=recs.rbegin(); it!=recs.rend(); ++it){
            if(it->timestamp<=timestamp){ anchor=&(*it); break; }
        }
        for(const auto& r : recs){
            if(anchor==nullptr || r.timestamp>=anchor->timestamp) keep.insert(r.id);
        }
    }

    return rewriteKeeping(keep, std::to_string(timestamp));
}

bool vecTable::purge(const std::vector<std::string>& pks, uint64_t timestamp){
    if(pks.empty()) return true; //nothing to do

    if(!file.is_open()){
        file.clear();
        file.open(meta.tablePath, std::ios::binary|std::ios::in|std::ios::out);
    }
    if(!file.is_open()){ std::cerr<<"Failed to open vec file for purge.\n"; return false; }

    std::unordered_set<std::string> toPurge(pks.begin(), pks.end());

    std::unordered_set<uint32_t> keep;
    file.clear();
    file.seekg(meta.metadataSize, std::ios::beg);
    for(uint32_t i=0;i<meta.recordCount;i++){
        uint32_t id;
        std::string padded(meta.pkSize, '\0');
        uint64_t ts;
        float embed[VEC_DIM];
        readBinary(file, id);
        file.read(&padded[0], meta.pkSize);
        readBinary(file, ts);
        file.read(reinterpret_cast<char*>(embed), sizeof(float)*VEC_DIM);
        if(!file){
            std::cerr<<"Corrupted record during vec purge.\n";
            return false;
        }
        std::string pk(padded.c_str(), strnlen(padded.c_str(), meta.pkSize));
        if(!toPurge.count(pk)) keep.insert(id);
    }

    return rewriteKeeping(keep, "purge_"+std::to_string(timestamp));
}