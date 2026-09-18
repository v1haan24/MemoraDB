#include "../storage/table.h"
#include <fstream>


bool Table::appendRecord(const Record& record){
    file.clear();
    file.seekp(0,std::ios::end);
    uint64_t offset=file.tellp();
    if(!file){ std::cerr<<"Failed to seek to end of file.\n"; return false;}
    uint64_t t=writeHeader(file,record.deleted);
    writePayload(file,meta,record.row);
    if(!file){
        std::cerr<<"Failed to write row to disk.\n";
        return false;
    }
    std::string pk=getPrimaryKey(record.row);
    history.addVersion(pk,{t,offset});
    return true;
}

bool Table::rollback(const std::string& pk,uint64_t timestamp){
    Record record=selectAsOf(pk,timestamp);
    if(record.row.values.empty()){ std::cerr<<"No version exists before given timestamp.\n"; return false;}
    return appendRecord(record);
}

bool Table::rollback(uint64_t timestamp){
    bool ok=true;
    for(const auto& pk:history.list()){
        Record record=selectAsOf(pk,timestamp);
        if(record.row.values.empty()) continue;
        if(!appendRecord(record)) ok=false;
    }
    return ok;
}