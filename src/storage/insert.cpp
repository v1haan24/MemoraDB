#include "table.h"
#include "serializer.h"
#include<iostream>
#include <chrono>
#include <cstring>
#include<climits>
#include <algorithm>
#include <fstream>

bool Table::insert(const Row& row){
    if(!validateRow(row)) return false;

    string pk=getPrimaryKey(row);
    if(history.count(pk)){cerr<<"Primary key '"<<pk<<"' already exists.\n"; return false;}

    fstream file("data/"+string(meta.name)+".db",ios::binary|ios::in|ios::out);
    if(!file){ cerr<<"Failed to open table file '"<<meta.name<<"'.\n"; return false; }

    file.seekp(0,ios::end); 
    uint64_t offset=file.tellp();
    uint64_t t=writeHeader(file);
    writePayload(file,row);
    if(!file){
    cerr<<"Failed to write row to disk.\n";
    file.close();
    return false;
    }
    history[pk].push_back({t,offset});
    meta.rowCount++;
    file.seekp(sizeof(int)+tns,ios::beg);
    writeBinary(file,meta.rowCount);
    file.close();
    return true;
}


bool Table::update(const Row& row){
    if(!validateRow(row)) return false;
    string pk=getPrimaryKey(row);
    if(!history.count(pk)){cerr<<"No row found with primary key '"<<pk<<"'.\n"; return false;}
    fstream file("data/"+string(meta.name)+".db",ios::binary|ios::in|ios::out);
    if(!file){ cerr<<"Failed to open table file '"<<meta.name<<"'.\n"; return false; }

    uint64_t prevOffset=history[pk].back().offset;
    file.seekg(prevOffset+sizeof(uint64_t),ios::beg);
    bool deleted;
    readBinary(file,deleted);
    if(deleted){
        cerr<<"Cannot update a deleted row.\n";
        file.close();
        return false;
    }

    file.seekp(0,ios::end); 
    uint64_t offset=file.tellp();
    uint64_t t=writeHeader(file);
    writePayload(file,row);
    if(!file){
    cerr<<"Failed to write row to disk.\n";
    file.close();
    return false;
    }
    history[pk].push_back({t,offset});
    file.close();
    return true;
}