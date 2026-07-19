#include "catalog.h"
#include "../serializer/serializer.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>
using namespace std;

bool Catalog::exist(const string& tableName){
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
        Serializer serializer;
        if(strnlen(table.name,tns)>=tns){cerr<<"Table name exceeds "<<tns-1<<" characters.\n"; return false;}
        if(table.columns.empty()){cerr<<"Table must contain at least one column.\n"; return false;}
        for(int i=0;i<table.columns.size();i++){
            for(int j=i+1;j<table.columns.size();j++){
                if(strcmp(table.columns[i].name,table.columns[j].name)==0){
                    cerr<<"Duplicate column name: "<<table.columns[i].name<<"\n";
                    return false;
                }
            }
        }
        int pkCount=0;
        for(const auto& col:table.columns){
            if(strnlen(col.name,cns)>=cns){cerr<<"Column name '"<<col.name<<"' exceeds "<<cns-1<<" characters.\n";return false;}
            if(col.type==STRING && col.size<=0){cerr<<"Invalid size for STRING column '"<<col.name<<"'.\n";return false;}
            if(col.isPK) pkCount++;
        }
        if(pkCount!=1){cerr<<"Exactly one primary key is required.\n";return false;}
        if(exist(table.name)){cerr<<"Table '"<<table.name<<"' already exists.\n"; return false;}
        
        CalcOffset(table);
        if(table.payloadSize==0){cerr<<"Payload size cannot be zero.\n"; return false;}

        ofstream file("data/"+string(table.name)+".db",ios::binary);
        if(!file){cerr<<"Failed to create file for table '"<<table.name<<"'.\n"; return false;}

        table.columnCount=table.columns.size();
        table.metadataSize=
                sizeof(int)+tns+sizeof(int)+sizeof(int)+sizeof(int)+
                table.columnCount*(
                cns+
                sizeof(DataType)+
                sizeof(int)+
                sizeof(int)+
                sizeof(bool)); 
        writeBinary(file,table.metadataSize);

        file.write(table.name,tns);
        writeBinary(file,table.rowCount);
        writeBinary(file,table.payloadSize);

        writeBinary(file,table.columnCount);
        for(const auto& col:table.columns) serializer.writeColumn(file,col);
        
        tables.emplace(table.name,Table(table));
        file.close();
        return true;
}

Table* Catalog::getTable(const string& tableName){
        auto it=tables.find(tableName);
        if(it==tables.end()) return nullptr;
        return &it->second;
}

TableMeta Catalog::readMetadata(const string& fileName){
        Serializer serializer;
        ifstream file("data/"+fileName,ios::binary);
        if(!file){cerr<<"Unable to open metadata file "<<fileName<<"\n"; return {}; }
        TableMeta temp;
        readBinary(file,temp.metadataSize);
        file.read(temp.name,tns);
        readBinary(file,temp.rowCount);
        readBinary(file,temp.payloadSize);

        readBinary(file,temp.columnCount);
        for(int i=0;i<temp.columnCount;i++){
            ColMeta col;
            serializer.readColumn(file,col);
            temp.columns.push_back(col);
        }
        return temp;
}

void Catalog::loadTables(){
    tables.clear();
    if(!filesystem::exists("data")) return;
    for(const auto& entry:filesystem::directory_iterator("data")){
            if(entry.path().extension()!=".db") continue;
            TableMeta meta=readMetadata(entry.path().filename().string());
            if(meta.name[0]=='\0') continue;
            tables.emplace(meta.name,Table(meta));
    }
}

void Catalog::showTables(){
    if(tables.empty()){
        cout<<"No tables found.\n";
        return;
    }
    for(auto &t:tables) cout<<t.first<<'\n';
}