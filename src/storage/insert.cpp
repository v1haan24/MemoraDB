#include "table.h"
#include <iostream>

bool Table::insert(const Row& row, const float* embedding){
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
    if (embedding && !writeEmbedding(pk, t, embedding)) {
        std::cerr << "Failed to store semantic vector for primary key '" << pk << "'.\n";
        return false;
    }
    return true;
}
