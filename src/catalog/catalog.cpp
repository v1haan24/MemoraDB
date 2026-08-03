#include "catalog.h"
#include <iostream>
#include <filesystem>
#include <cstring>
#include "../storage/table.h"

bool Catalog::exist(const std::string& tableName){
    return tables.count(tableName);
}

void Catalog::CalcOffset(TableMeta& table){
        int size=0;
        for(auto &c:table.columns){
            c.offset=size;
            size+=c.size;
        }
        table.payloadSize=size;
}

bool Catalog::createTable(TableMeta& table){
        if(strnlen(table.name,tns)>=tns){std::cerr<<"Table name exceeds "<<tns-1<<" characters.\n"; return false;}
        if(table.columns.empty()){std::cerr<<"Table must contain at least one column.\n"; return false;}
        for(int i=0;i<table.columns.size();i++){
            for(int j=i+1;j<table.columns.size();j++){
                if(strcmp(table.columns[i].name,table.columns[j].name)==0){
                    std::cerr<<"Duplicate column name: "<<table.columns[i].name<<"\n";
                    return false;
                }
            }
        }
        int pkCount=0;
        for(const auto& col:table.columns){
            if(strnlen(col.name,cns)>=cns){std::cerr<<"Column name '"<<col.name<<"' exceeds "<<cns-1<<" characters.\n";return false;}
            if(col.type==STRING && col.size<=0){std::cerr<<"Invalid size for STRING column '"<<col.name<<"'.\n";return false;}
            if(col.isPK) pkCount++;
        }
        if(pkCount!=1){std::cerr<<"Exactly one primary key is required.\n";return false;}
        if(exist(table.name)){std::cerr<<"Table '"<<table.name<<"' already exists.\n"; return false;}
        
        CalcOffset(table);
        if(table.payloadSize==0){std::cerr<<"Payload size cannot be zero.\n"; return false;}

        std::ofstream file("data/"+std::string(table.name)+".db",std::ios::binary);
        if(!file){std::cerr<<"Failed to create file for table '"<<table.name<<"'.\n"; return false;}

        table.columnCount=table.columns.size();
        table.metadataSize=
            sizeof(int)+tns+sizeof(int)+sizeof(int)+
            table.columnCount*(cns+sizeof(DataType)+sizeof(int)+sizeof(int)+sizeof(bool));
            
        writeBinary(file,table.metadataSize);

        file.write(table.name,tns);
        //writeBinary(file,table.rowCount);
        writeBinary(file,table.payloadSize);

        writeBinary(file,table.columnCount);
        for(const auto& col:table.columns) writeColumn(file,col);
        
        tables.emplace(std::string(table.name),std::move(Table(table)));
        file.close();
        return true;
}

Table* Catalog::getTable(const std::string& tableName){
        auto it=tables.find(tableName);
        if(it==tables.end()) return nullptr;
        return &it->second;
}

TableMeta Catalog::readMetadata(const std::string& fileName){
        std::ifstream file("data/"+fileName,std::ios::binary);
        if(!file){std::cerr<<"Unable to open metadata file "<<fileName<<"\n"; return {}; }
        TableMeta temp;
        readBinary(file,temp.metadataSize);
        file.read(temp.name,tns);
        //readBinary(file,temp.rowCount);
        readBinary(file,temp.payloadSize);

        readBinary(file,temp.columnCount);
        for(int i=0;i<temp.columnCount;i++){
            ColMeta col;
            readColumn(file,col);
            temp.columns.push_back(col);
        }
        return temp;
}

void Catalog::loadTables(){
    tables.clear();
    if(!std::filesystem::exists("data")) return;
    for(const auto& entry:std::filesystem::directory_iterator("data")){
            if(entry.path().extension()!=".db") continue;
            TableMeta meta=readMetadata(entry.path().filename().string());
            if(meta.name[0]=='\0') continue;
            tables.emplace(meta.name,Table(meta));
    }
}

void Catalog::showTables(){
    if(tables.empty()){
        std::cout<<"No tables found.\n";
        return;
    }
    for(auto &t:tables) std::cout<<t.first<<'\n';
}