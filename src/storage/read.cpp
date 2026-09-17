#include "table.h"

std::string Table::getPrimaryKey(const Row& row){
    for(int i=0;i<meta.columnCount;i++){
        if(!meta.columns[i].isPK) continue;
        if(meta.columns[i].type==INT) return std::to_string(std::stoi(row.values[i]));
        if(meta.columns[i].type==FLOAT) return std::to_string(std::stof(row.values[i]));
        return row.values[i];
    }
    std::cerr<<"Primary key not found.\n";
    return "";
}

Record Table::readRecord(uint64_t offset){
    file.clear();
    file.seekg(offset,std::ios::beg);
    if(!file){ std::cerr<<"Failed to seek to record.\n"; return {};}
    Record record;
    readBinary(file,record.timestamp);
    readBinary(file,record.deleted);
    if(!file){ std::cerr<<"Failed to read record header.\n"; return {};}
    record.row=readPayload(file,meta);
    if(!file){ std::cerr<<"Failed to read record payload.\n";return {};}
    return record;
}

Record Table::latest(const std::string& pk){
    if(!history.contains(pk)){std::cerr << "No row found.\n"; return {}; }
    return readRecord(history.latest(pk).offset);
}

std::vector<Record> Table::scanLatest(){
    std::vector<Record> records;
    for(const auto& pk:history.list()){
        Record record=readRecord(history.latest(pk).offset);
        if(!record.deleted) records.push_back(record);
    }
    return records;
}

bool Table::isDeleted(uint64_t offset){
    file.clear();
    file.seekg(offset,std::ios::beg);
    if(!file){ std::cerr<<"Failed to seek to record.\n"; return true;}
    uint64_t timestamp;
    bool deleted;
    readBinary(file,timestamp);
    readBinary(file,deleted);
    if(!file){ std::cerr<<"Failed to read record header.\n"; return true;}
    return deleted;
}

std::vector<VCandidate> Table::scanLatestKeys(){
    std::vector<VCandidate> keys;
    for(const auto& pk:history.list()){
        const RecordVersion& v=history.latest(pk);
        if(isDeleted(v.offset)) continue;
        keys.push_back({pk,v.timestamp});
    }
    return keys;
}

std::vector<VCandidate> Table::activeKeys(){
    std::vector<VCandidate> keys;
    for(const auto& pk:history.list()){
        for(const auto& v:history.getHistory(pk)) keys.push_back({pk,v.timestamp});
    }
    return keys;
}