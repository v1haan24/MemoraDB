#include "table.h"
#include <iostream>

bool Table::insert(const Row& row){
    if(!validateRow(row)) return false;

    std::string pk=getPrimaryKey(row);
    if(history.contains(pk)){std::cerr<<"Primary key '"<<pk<<"' already exists.\n"; return false;}

    file.clear();
    file.seekp(0,std::ios::end); 
    uint64_t offset=file.tellp();
    if(!file){
        std::cerr<<"Failed to seek to end of file.\n";
        return false;
    }
    
    uint64_t t=writeHeader(file,false);
    writePayload(file,meta,row);
    if(!file){
        std::cerr<<"Failed to write row to disk.\n";
        return false;
    }
    history.addVersion(pk,{t, offset});
    return true;
}

bool Table::insertSemantic(std::string pk,uint64_t timestamp,const Row& row){
    std::string temp = "";
    
}