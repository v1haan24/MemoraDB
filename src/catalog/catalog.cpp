#include "catalog.h"
#include "../storage/serializer.h"
#include <iostream>
#include <fstream>
#include <unordered_set>
#include <cstring>
using namespace std;

bool Catalog::exist(const string& tableName){
        for(auto &t:tables) if(strcmp(t.name,tableName.c_str())==0) return true;
        return false;
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
        if(strnlen(table.name,tns)>=tns){cerr<<"Table name exceeds "<<tns-1<<" characters.\n"; return false;}
        if(table.columns.empty()){cerr<<"Table must contain at least one column.\n"; return false;}
        unordered_set<string> names;
        int pkCount=0;
        for(const auto& col:table.columns){
            if(strnlen(col.name,cns)>=cns){cerr<<"Column name '"<<col.name<<"' exceeds "<<cns-1<<" characters.\n";return false;}
            if(!names.insert(col.name).second){cerr<<"Duplicate column name: "<<col.name<<"\n"; return false;}
            if(col.type==STRING && col.size<=0){cerr<<"Invalid size for STRING column '"<<col.name<<"'.\n";return false;}
            if(col.isPK) pkCount++;
        }
        if(pkCount!=1){cerr<<"Exactly one primary key is required.\n";return false;}
        if(exist(table.name)){cerr<<"Table '"<<table.name<<"' already exists.\n"; return false;}
        ifstream check("data/"+string(table.name)+".db",ios::binary);
        if(check){ cerr<<"Table '"<<table.name<<"' already exists.\n"; return false;}


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
        for(const auto& col:table.columns) writeColumn(file,col);
        
        tables.push_back(table);
        return true;
}

TableMeta* Catalog::getTable(const string& tableName){
        for(auto &t:tables){
            if(strcmp(t.name,tableName.c_str())==0) return &t;
       }
       return nullptr;
}

TableMeta Catalog::readMetadata(string fileName){
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
            readColumn(file,col);
            temp.columns.push_back(col);
        }
        return temp;
}