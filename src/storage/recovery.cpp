#include "table.h"
#include <iostream>

void Table::recoverState(){
    file.clear();

    int pk=-1;
    for(int i=0;i<meta.columnCount;i++){
        if(meta.columns[i].isPK){ pk=i; break; }
    }
    if(pk==-1){ std::cerr<<"Table '"<<meta.name<<"' has no primary key column.\n";return; }

    file.seekg(meta.metadataSize,std::ios::beg);
    while(true){
        uint64_t recordStart=file.tellg();

        uint64_t timestamp;
        readBinary(file,timestamp);
        if(!file){
            if(!file.eof()) std::cerr<<"Corrupted record encountered during recovery.\n";
            break;
        }
        bool deleted;
        readBinary(file,deleted);
        if(!file){std::cerr << "Corrupted record encountered during recovery.\n";break;}
        file.seekg(recordStart+rhsz+meta.columns[pk].offset,std::ios::beg);
        if(!file){std::cerr << "Corrupted record encountered during recovery.\n";break;}
        
        std::string primaryKey;
        if(meta.columns[pk].type==INT){
            int x;
            readBinary(file,x);
            if(!file){std::cerr << "Corrupted record encountered during recovery.\n";break;}
            primaryKey=std::to_string(x);
        }
        else if(meta.columns[pk].type==FLOAT){
            float x;
            readBinary(file,x);
            if(!file){std::cerr << "Corrupted record encountered during recovery.\n";break;}
            primaryKey=std::to_string(x);
        }
        else if(meta.columns[pk].type==BOOL){
            bool x;
            readBinary(file,x);
            if(!file){std::cerr << "Corrupted record encountered during recovery.\n";break;}
            primaryKey=x?"true":"false";
        }
        else if(meta.columns[pk].type==STRING){
            std::string temp(meta.columns[pk].size,'\0');
            file.read(&temp[0],meta.columns[pk].size);
            if(!file){std::cerr << "Corrupted record encountered during recovery.\n";break;}
            temp.resize(strnlen(temp.c_str(),meta.columns[pk].size));
            primaryKey=temp;
        }
        history.addVersion(primaryKey,{timestamp,recordStart});
        file.seekg(recordStart+rhsz+meta.payloadSize,std::ios::beg);
    }
    
}