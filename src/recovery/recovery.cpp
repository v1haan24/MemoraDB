#include "recovery.h"
#include "../serializer/serializer.h"
#include <iostream>
#include <string>
using namespace std;

void Recovery::recoverState(TableMeta& meta,HistoryIndex& history){
    fstream file("data/"+string(meta.name)+".db",ios::binary|ios::in);
    if(!file){ cerr<<"Failed to open table file '"<<meta.name<<"' for recovery.\n"; return;}

    meta.rowCount=0;
    int pk=-1;
    for(int i=0;i<meta.columnCount;i++){
        if(meta.columns[i].isPK){ pk=i; break; }
    }
    if(pk==-1){ cerr<<"Table '"<<meta.name<<"' has no primary key column.\n"; file.close(); return; }

    file.seekg(meta.metadataSize,ios::beg);
    while(true){
        uint64_t recordStart=file.tellg();

        uint64_t timestamp;
        readBinary(file,timestamp);
        if(!file){
            if(!file.eof()) cerr<<"Corrupted record encountered during recovery.\n";
            break;
        }
        bool deleted;
        readBinary(file,deleted);
        file.seekg(recordStart+rhsz+meta.columns[pk].offset,ios::beg);
       
        string primaryKey;
        if(meta.columns[pk].type==INT){
            int x;
            readBinary(file,x);
            primaryKey=to_string(x);
        }
        else if(meta.columns[pk].type==FLOAT){
            float x;
            readBinary(file,x);
            primaryKey=to_string(x);
        }
        else if(meta.columns[pk].type==BOOL){
            bool x;
            readBinary(file,x);
            primaryKey=x?"true":"false";
        }
        else if(meta.columns[pk].type==STRING){
            string temp(meta.columns[pk].size,'\0');
            file.read(&temp[0],meta.columns[pk].size);
            temp.resize(strnlen(temp.c_str(),meta.columns[pk].size));
            primaryKey=temp;
        }
        history.addVersion(primaryKey,{timestamp,recordStart});
        file.seekg(recordStart+rhsz+meta.payloadSize,ios::beg);
    }

    meta.rowCount=history.size();
    file.close();
}