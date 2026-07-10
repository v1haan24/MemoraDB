#include "table.h"
#include "serializer.h"
#include <chrono>
#include <cstring>
#include<climits>
#include <algorithm>
#include <fstream>

bool Table::insert(const Row& row){
    if(!validateRow(row)) return false;

    string pk=getPrimaryKey(row);
    if(history.count(pk)) return false;

    fstream file("data/"+string(meta.name)+".db",ios::binary|ios::in|ios::out);
    if(!file) return false;

    file.seekp(0,ios::end); 
    uint64_t offset=file.tellp();
    uint64_t t=writeHeader(file);
    writePayload(file,row);

    history[pk].push_back({t,offset});
    meta.rowCount++;
    file.seekp(sizeof(int)+tns,ios::beg);
    writeBinary(file,meta.rowCount);
    file.close();
    return true;
}