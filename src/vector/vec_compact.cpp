#include "vecTable.h"
#include <iostream>
#include <cstdio>
#include <filesystem>
#include <unordered_set>

bool vecTable::writeRewriteHeader(std::fstream& out){
    writeBinary(out, meta.metadataSize);
    out.write(meta.name, tns);
    writeBinary(out, meta.pkSize);
    uint32_t placeholderCount=0;
    writeBinary(out, placeholderCount);
    writeBinary(out, meta.payloadSize);
    return (bool)out;
}
bool vecTable::finalizeRewrite(std::fstream& out, const std::string& outPath, uint32_t recordCount, const std::string& archiveTag){
    out.seekp(sizeof(int)+tns+sizeof(int), std::ios::beg);
    writeBinary(out, recordCount);
    if(!out){
        std::cerr<<"Failed to patch record count in compact vec file.\n";
        out.close();
        std::remove(outPath.c_str());
        return false;
    }
    out.close();
    file.close();
    std::string tableName=meta.tablePath.parent_path().filename().string();
    std::string archivePath="data/"+tableName+"/archive/archive_vec_"+archiveTag+".vec";
    std::filesystem::create_directories("data/"+tableName+"/archive");
    if(std::filesystem::exists(archivePath)){
        std::cerr<<"Vector archive already exists.\n";
        std::remove(outPath.c_str());
        file.open(meta.tablePath, std::ios::binary|std::ios::in|std::ios::out);
        return false;
    }
    if(std::rename(meta.tablePath.string().c_str(), archivePath.c_str())!=0){
        std::cerr<<"Failed to archive old vector file.\n";
        file.open(meta.tablePath, std::ios::binary|std::ios::in|std::ios::out);
        return false;
    }
    if(std::rename(outPath.c_str(), meta.tablePath.string().c_str())!=0){
        std::cerr<<"Failed to activate rewritten vector file.\n";
        std::rename(archivePath.c_str(), meta.tablePath.string().c_str());
        file.open(meta.tablePath, std::ios::binary|std::ios::in|std::ios::out);
        return false;
    }
    meta.recordCount=recordCount;
    file.open(meta.tablePath, std::ios::binary|std::ios::in|std::ios::out);
    if(!file.is_open()){
        std::cerr<<"ERROR: failed to reopen vec file after rewrite.\n";
        return false;
    }
    return true;
}
bool vecTable::rewriteKeeping(const std::unordered_set<uint32_t>& keep, const std::string& archiveTag){
    if(!file.is_open()){
        file.clear();
        file.open(meta.tablePath, std::ios::binary|std::ios::in|std::ios::out);
    }
    if(!file.is_open()){ std::cerr<<"Failed to open vec file.\n"; return false; }

    std::string rewriteTempPath=meta.tablePath.string()+".tmp";
    std::fstream out(rewriteTempPath, std::ios::binary|std::ios::in|std::ios::out|std::ios::trunc);
    if(!out.is_open()){ std::cerr<<"Failed to create compact vec file.\n"; return false; }
    if(!writeRewriteHeader(out)){
        std::cerr<<"Failed to write compact vec header.\n";
        out.close();
        std::remove(rewriteTempPath.c_str());
        return false;
    }

    file.clear();
    file.seekg(meta.metadataSize, std::ios::beg);
    uint32_t rewriteId=0;
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
            std::remove(rewriteTempPath.c_str());
            return false;
        }
        if(!keep.count(id)) continue;

        writeBinary(out, rewriteId);
        out.write(padded.data(), meta.pkSize);
        writeBinary(out, ts);
        out.write(reinterpret_cast<const char*>(embed), sizeof(float)*VEC_DIM);
        rewriteId++;
    }
    if(!out){
        std::cerr<<"Failed to write rewritten vec file.\n";
        out.close();
        std::remove(rewriteTempPath.c_str());
        return false;
    }
    return finalizeRewrite(out, rewriteTempPath, rewriteId, archiveTag);
}
bool vecTable::purge(const std::vector<std::string>& pks, uint64_t timestamp){
    if(pks.empty()) return true; 

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
bool vecTable::startRewrite(){
    if(rewriting){
        std::cerr<<"ERROR: a vector compaction rewrite is already in progress.\n";
        return false;
    }
    if(!file.is_open()){
        file.clear();
        file.open(meta.tablePath, std::ios::binary|std::ios::in|std::ios::out);
    }
    if(!file.is_open()){ std::cerr<<"Failed to open vec file for compaction.\n"; return false; }
    tempPath = meta.tablePath.string()+".tmp";
    tempFile.open(tempPath, std::ios::binary|std::ios::in|std::ios::out|std::ios::trunc);
    if(!tempFile.is_open()){ std::cerr<<"Failed to create compact vec file.\n"; return false; }

    if(!writeRewriteHeader(tempFile)){
        std::cerr<<"Failed to write compact vec header.\n";
        tempFile.close();
        std::remove(tempPath.c_str());
        return false;
    }
    newId = 0;
    rewriting = true;
    return true;
}
bool vecTable::copyRecord(uint32_t oldId){
    if(!rewriting){
        std::cerr<<"ERROR: copyRecord() called with no compaction rewrite in progress.\n";
        return false;
    }
    if(oldId == UINT32_MAX) return true;  
    if(oldId >= meta.recordCount){
        std::cerr<<"ERROR: copyRecord id "<<oldId<<" out of range.\n";
        return false;
    }
    file.clear();
    uint64_t offset = (uint64_t)meta.metadataSize + (uint64_t)oldId*(uint64_t)meta.payloadSize;
    file.seekg(offset, std::ios::beg);
    if(!file){ std::cerr<<"ERROR: failed to seek to record "<<oldId<<" during compaction copy.\n"; return false; }
    uint32_t id;
    std::string padded(meta.pkSize, '\0');
    uint64_t ts;
    float embed[VEC_DIM];
    readBinary(file, id);
    file.read(&padded[0], meta.pkSize);
    readBinary(file, ts);
    file.read(reinterpret_cast<char*>(embed), sizeof(float)*VEC_DIM);
    if(!file){ std::cerr<<"ERROR: failed to read record "<<oldId<<" during compaction copy.\n"; return false; }

    writeBinary(tempFile, newId);
    tempFile.write(padded.data(), meta.pkSize);
    writeBinary(tempFile, ts);
    tempFile.write(reinterpret_cast<const char*>(embed), sizeof(float)*VEC_DIM);
    if(!tempFile){ std::cerr<<"ERROR: failed to write copied record during compaction.\n"; return false; }

    newId++;
    return true;
}
bool vecTable::finishRewrite(const std::string& archiveTag){
    if(!rewriting){
        std::cerr<<"ERROR: finishRewrite() called with no compaction rewrite in progress.\n";
        return false;
    }
    bool ok = finalizeRewrite(tempFile, tempPath, newId, archiveTag);
    rewriting = false;
    return ok;
}
void vecTable::cancelRewrite(){
    if(!rewriting) return;
    tempFile.close();
    std::remove(tempPath.c_str());
    rewriting = false;
}