#include "table.h"
#include "serializer.h"
#include <chrono>
#include <cstring>
#include<climits>
#include <algorithm>
#include <fstream>

Row Table::readRow(uint64_t offset){
    fstream file("data/"+string(meta.name)+".db",ios::binary|ios::in);
    if(!file) return {};
    
    file.seekg(offset,ios::beg);
    uint64_t timestamp;
    bool deleted;
    readBinary(file,timestamp);
    readBinary(file,deleted);
    //if(deleted){ file.close(); return {}; }
    Row row=readPayload(file);
    file.close();
    return row;
}

Row Table::latest(const string& pk){
    if(!history.count(pk)) return {};
    return readRow(history[pk].back().offset);
}