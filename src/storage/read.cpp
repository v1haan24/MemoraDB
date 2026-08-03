#include "table.h"

std::string Table::getPrimaryKey(const Row& row){
        for(int i=0;i<meta.columnCount;i++){
            if(meta.columns[i].isPK) return row.values[i];
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