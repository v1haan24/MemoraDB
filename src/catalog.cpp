#include "catalog.h"
#include "serializer.h"
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
        if(strlen(table.name)>=tns) return false;
        if(table.columns.empty()) return false;
        unordered_set<string> names;
        int pkCount=0;
        for(const auto& col:table.columns){
            if(strlen(col.name)>=cns) return false;
            if(!names.insert(col.name).second) return false;
            if(col.type==STRING && col.size<=0) return false;
            if(col.isPK) pkCount++;
        }
        if(pkCount!=1) return false;
        if(exist(table.name)) return false;


        CalcOffset(table);
        if(table.payloadSize==0) return false;

        ofstream file("data/"+string(table.name)+".db",ios::binary);
        if(!file) return false;

        table.columnCount=table.columns.size();
        table.metadataSize=
                sizeof(int)+tns+sizeof(int)+sizeof(int)+sizeof(int)+
                table.columnCount*(
                cns+
                sizeof(DataType)+
                sizeof(int)+
                sizeof(int)+
                sizeof(bool)); //<--- if change in cns,tns change this too
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
        if(!file) return {};
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