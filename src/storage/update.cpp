#include "table.h"
#include <iostream>
#include <fstream>
using namespace std;

bool Table::update(const Row& row){
    if(!validator.validateRow(row,meta)) return false;
    string pk=getPrimaryKey(row);
    if(!history.contains(pk)){cerr<<"No row found with primary key '"<<pk<<"'.\n"; return false;}
    fstream file("data/"+string(meta.name)+".db",ios::binary|ios::in|ios::out);
    if(!file){ cerr<<"Failed to open table file '"<<meta.name<<"'.\n"; return false; }

    uint64_t prevOffset=history.latest(pk).offset;
    file.seekg(prevOffset+rhsz-sizeof(bool),ios::beg);
    bool deleted;
    readBinary(file,deleted);
    if(deleted){
        cerr<<"Cannot update a deleted row.\n";
        file.close();
        return false;
    }

    file.seekp(0,ios::end); 
    uint64_t offset=file.tellp();
    uint64_t t=serializer.writeHeader(file,false);
    serializer.writePayload(file,meta,row);
    if(!file){
        cerr<<"Failed to write row to disk.\n";
        file.close();
        return false;
    }
    history.addVersion(pk,{t, offset});
    file.close();
    return true;
}