#include "table.h"
#include "serializer.h"
#include <chrono>
#include <cstring>
#include <climits>
#include <algorithm>
#include <fstream>
#include <iostream>

bool Table::Delete(const string& pk)
{
    if(history.empty()) recoverState();

    if(history.find(pk) == history.end())
    {
        cerr<<"KEY was nevere inserted!! KEY doesn't exist!!"<<endl;
        return false;
    }

    
    uint64_t offset = history[pk].back().offset;
    fstream file("data/"+string(meta.name)+".db",ios::binary|ios::in|ios::out);
    if(!file)
    {
        cerr<<"Couldn't open file!!"<<endl;
        return false;
    } 

    file.seekg(offset+sizeof(uint64_t), ios::beg);
    bool deleted;
    readBinary(file, deleted);
    Row row = readPayload(file);

    if(deleted)
    {
        cerr<<"Corresponding row already DELETED!!"<<endl;
        file.close();
        return false;
    }

    file.seekp(0, ios::end);
    uint64_t new_offset=file.tellp();
    uint64_t timestamp = writeHeader(file, true);
    writePayload(file, row);

    history[pk].push_back({timestamp, new_offset});

    file.close();
    return true;
}