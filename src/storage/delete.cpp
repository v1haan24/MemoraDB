#include "table.h"
#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

bool Table::deleteRow(const string& pk){
    if(!history.contains(pk)){ cerr<<"No row found with primary key '"<<pk<<"'.\n"; return false;}
    
    uint64_t prevOffset=history.latest(pk).offset;
    fstream file("data/"+string(meta.name)+".db",ios::binary|ios::in|ios::out);
    if(!file){cerr<<"Failed to open table file '"<<meta.name<<"'.\n"; return false;}
    
    file.seekg(prevOffset+sizeof(uint64_t),ios::beg);
    bool deleted;
    readBinary(file,deleted);
    if(deleted){cerr<<"Row with primary key '"<<pk<<"' is already deleted.\n"; return false;}
    
    vector<char> temp(meta.payloadSize);
    file.read(temp.data(),meta.payloadSize);
    file.seekp(0,ios::end);
    uint64_t offset=file.tellp();
    uint64_t t=writeHeader(file,true);
    file.write(temp.data(),meta.payloadSize);
    history.addVersion(pk,{t, offset});

    file.close();
    return true;
}