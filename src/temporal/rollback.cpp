#include "../storage/table.h"
#include <fstream>
using namespace std;

bool Table::appendRecord(const Record& record){
    fstream file("data/"+string(meta.name)+".db",ios::binary|ios::in|ios::out);
    if(!file){
        cerr<<"Failed to open table file '"<<meta.name<<"'.\n";
        return false;
    }
    file.seekp(0,ios::end);
    uint64_t offset=file.tellp();
    uint64_t t=writeHeader(file,record.deleted);
    writePayload(file,meta,record.row);
    if(!file){
        cerr<<"Failed to write row to disk.\n";
        file.close();
        return false;
    }
    string pk=getPrimaryKey(record.row);
    history.addVersion(pk,{t,offset});
    file.close();
    return true;
}

bool Table::rollback(const string& pk,uint64_t timestamp){
    Record record=selectAsOf(pk,timestamp);
    if(record.row.values.empty()){ cerr<<"No version exists before given timestamp.\n"; return false;}
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