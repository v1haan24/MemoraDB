#include "serializer.h"
#include "table.h"
#include <iostream>
using namespace std;

void Table::recoverState(){
    fstream file("data/"+string(meta.name)+".db",ios::binary|ios::in);
    if(!file) return;

    meta.rowCount=0;
    int pk=-1;
    for(int i=0;i<meta.colCount;i++){
        if(meta.columns[i].isPK){ pk=i; break; }
    }
    if(pk==-1){ file.close(); return; }

    file.seekg(meta.metadataSize,ios::beg);
    while(true){
        uint64_t recordStart=file.tellg();

        uint64_t timestamp;
        readBinary(file,timestamp);
        if(!file) break;
        bool deleted;
        readBinary(file,deleted);
        file.seekg(recordStart+row_hdsize+meta.columns[pk].offset,ios::beg);
       
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
            string temp(meta.columns[pk].colSize,'\0');
            file.read(&temp[0],meta.columns[pk].colSize);
            temp.resize(strlen(temp.c_str()));
            primaryKey=temp;
        }
        history[primaryKey].push_back({timestamp,recordStart});
        file.seekg(recordStart+row_hdsize+meta.payloadSize,ios::beg);
    }

    meta.rowCount=(int)history.size();
    file.close();
}