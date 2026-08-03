#include "table.h"
#include <iostream>
#include <vector>

bool Table::deleteRow(const std::string& pk){
    if(!history.contains(pk)){ std::cerr<<"No row found with primary key '"<<pk<<"'.\n"; return false;}
    
    uint64_t prevOffset=history.latest(pk).offset;
    file.clear();
    
    file.seekg(prevOffset+sizeof(uint64_t),std::ios::beg);
    bool deleted;
    readBinary(file,deleted);
    if(!file){ std::cerr<<"Failed to read delete flag.\n"; return false;}
    if(deleted){std::cerr<<"Row with primary key '"<<pk<<"' is already deleted.\n"; return false;}
    
    std::vector<char> temp(meta.payloadSize);
    file.read(temp.data(),meta.payloadSize);
    if(!file){ std::cerr<<"Failed to read row payload.\n"; return false;}
    file.clear();
    file.seekp(0,std::ios::end);
    uint64_t offset=file.tellp();
    if(!file){ std::cerr<<"Failed to seek to end of file.\n"; return false;}
    uint64_t t=writeHeader(file,true);
    file.write(temp.data(),meta.payloadSize);
    if(!file){ std::cerr<<"Failed to write deleted record.\n"; return false;}
    history.addVersion(pk,{t, offset});

   return true;
}