#include "table.h"
#include <fstream>
#include <vector>
#include <chrono>
#include "serializer.h"

bool Table::deleteRow(const string& pk){
    if(!history.count(pk)) return false;
    uint64_t prevOffset=history[pk].back().offset;

    fstream file("data/"+string(meta.name)+".db",ios::binary|ios::in|ios::out);
    if(!file) return false;
    file.seekg(prevOffset+sizeof(uint64_t),ios::beg);

    bool deleted;
    readBinary(file,deleted);
    if(deleted){
        file.close();
        return false;
    }

    vector<char> temp(meta.payloadSize);
    file.seekg(prevOffset+rhsz,ios::beg);
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

    if(!file){
        file.close();
        return false;
    }

    history[pk].push_back({t,offset});

    file.close();
    return true;
}