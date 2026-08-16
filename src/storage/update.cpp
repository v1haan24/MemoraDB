#include "table.h"
#include <iostream>

bool Table::update(const Row& row){
    if(!validateRow(row)) return false;
    std::string pk=getPrimaryKey(row);
    if(!history.contains(pk)){std::cerr<<"No row found with primary key '"<<pk<<"'.\n"; return false;}
    file.clear();

    uint64_t prevOffset=history.latest(pk).offset;
    file.seekg(prevOffset+rhsz-sizeof(bool),std::ios::beg);
    bool deleted;
    readBinary(file,deleted);
    if(!file){std::cerr<<"Failed to read delete flag.\n"; return false;}
    if(deleted){ std::cerr<<"Cannot update a deleted row.\n"; return false;}
    file.clear();
    file.seekp(0,std::ios::end); 
    uint64_t offset=file.tellp();
    uint64_t t=writeHeader(file,false);
    writePayload(file,meta,row);
    if(!file){
        std::cerr<<"Failed to write row to disk.\n";
        return false;
    }
    history.addVersion(pk,{t, offset});
    writeQueue(pk,t,row); 
    return true;
}