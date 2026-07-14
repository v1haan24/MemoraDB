#include "table.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <chrono>
#include "serializer.h"

bool Table::deleteRow(const string& pk){
    auto it=history.find(pk);
    if(it==history.end()){cerr<<"No row found with primary key '"<<pk<<"'.\n"; return false;}
    uint64_t prevOffset=it->second.back().offset;

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
    uint64_t t=
        chrono::duration_cast<chrono::milliseconds>(
            chrono::system_clock::now().time_since_epoch()
        ).count();

    deleted=true;
    writeBinary(file,t);
    writeBinary(file,deleted);
    file.write(temp.data(),meta.payloadSize);
    it->second.push_back({t,offset});

    file.close();
    return true;
}