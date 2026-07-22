#include "table.h"
#include <fstream>
using namespace std;

string Table::getPrimaryKey(const Row& row){
        for(int i=0;i<meta.columnCount;i++){
            if(meta.columns[i].isPK) return row.values[i];
        }
        cerr<<"Primary key not found.\n";
        return "";
}

Record Table::readRecord(uint64_t offset){
    fstream file("data/"+string(meta.name)+".db",ios::binary|ios::in);
    if(!file){
        cerr<<"Failed to open table file '"<<meta.name<<"'.\n";
        return {};
    }
    file.seekg(offset,ios::beg);
    Record record;
    readBinary(file,record.timestamp);
    readBinary(file,record.deleted);
    record.row=readPayload(file,meta);
    file.close();
    return record;
}

Record Table::latest(const string& pk){
    if(!history.contains(pk)){cerr << "No row found.\n"; return {}; }
    return readRecord(history.latest(pk).offset);
}