#include "table.h"
#include "../vector/vecTable.h"
#include "../vector/semantic/vector_index.h"
#include <cstdio>
#include <filesystem>
#include <unordered_set>

bool Table::compact(uint64_t timestamp, vecTable* vt, VectorIndex* idx){
    bool hasVector = (vt != nullptr && idx != nullptr);
    if(hasVector) idx->buildIndex(*vt);

    std::unordered_set<uint64_t> keep;
    for(const auto& pk:history.list()){
        const std::vector<RecordVersion>& versions=history.getHistory(pk);
        const RecordVersion* anchor=history.latestBefore(pk,timestamp);
        for(const auto& v:versions){
            if(anchor==nullptr || v.timestamp>=anchor->timestamp){
                keep.insert(v.offset);
            }
        }
    }
    if(hasVector){
        if(!vt->startRewrite()){
            std::cerr<<"Failed to begin vector compaction rewrite.\n";
            return false;
        }
    }

    std::string tempPath=filePath+".tmp";
    std::fstream out(tempPath,std::ios::binary|std::ios::in|std::ios::out|std::ios::trunc);
    if(!out.is_open()){
        std::cerr<<"Failed to create compact file.\n";
        if(hasVector) vt->cancelRewrite();
        return false;
    }

    if(!writeMetadata(out,meta)){
        std::cerr<<"Failed to write metadata.\n";
        out.close();
        std::remove(tempPath.c_str());
        if(hasVector) vt->cancelRewrite();
        return false;
    }

    HistoryIndex newHistory;
    file.clear();
    file.seekg(meta.metadataSize,std::ios::beg);
    while(file.peek()!=EOF){
        uint64_t offset=file.tellg();

        uint64_t recordTime; bool deleted;
        readBinary(file,recordTime);
        readBinary(file,deleted);
        Row row=readPayload(file,meta);
        if(!file){
            std::cerr<<"Corrupted record during compaction.\n";
            out.close();
            std::remove(tempPath.c_str());
            if(hasVector) vt->cancelRewrite();
            openFile();
            return false;
        }

        if(keep.count(offset)){
            uint64_t newOffset=out.tellp();
            writeBinary(out,recordTime);
            writeBinary(out,deleted);
            writePayload(out,meta,row);
            std::string pk=getPrimaryKey(row);
            newHistory.addVersion(pk,{recordTime,newOffset});

            if(hasVector){
                uint32_t oldVecId=idx->findId(pk,recordTime);
                if(!vt->copyRecord(oldVecId)){
                    std::cerr<<"Failed to copy vector record during compaction.\n";
                    out.close();
                    std::remove(tempPath.c_str());
                    vt->cancelRewrite();
                    openFile();
                    return false;
                }
            }
        }
    }
    if(!out){
        std::cerr<<"Failed to write compacted file.\n";
        out.close();
        std::remove(tempPath.c_str());
        if(hasVector) vt->cancelRewrite();
        openFile();
        return false;
    }
    out.close();

    file.close();
    std::string archivePath="data/"+std::string(meta.name)+"/archive/archive_"+std::to_string(timestamp)+".db";
    std::filesystem::create_directories("data/"+std::string(meta.name)+"/archive");
    if(std::filesystem::exists(archivePath)){
        std::cerr<<"Archive already exists.\n";
        std::remove(tempPath.c_str());
        if(hasVector) vt->cancelRewrite();
        openFile();
        return false;
    }
    if(std::rename(filePath.c_str(),archivePath.c_str())!=0){
        std::cerr<<"Failed to archive old database.\n";
        if(hasVector) vt->cancelRewrite();
        openFile();
        return false;
    }
    if(std::rename(tempPath.c_str(),filePath.c_str())!=0){
        std::cerr<<"Failed to activate compacted database.\n";
        std::rename(archivePath.c_str(),filePath.c_str());
        if(hasVector) vt->cancelRewrite();
        openFile();
        return false;
    }
    history=newHistory;
    openFile();

    if(hasVector && !vt->finishRewrite(std::to_string(timestamp))){
        std::cerr<<"Vector table compaction failed.\n";
        return false;
    }
    return true;
}