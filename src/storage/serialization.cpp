#include "table.h"
#include <chrono>
#include <cstring>
#include <fstream>

void writeColumn(ostream& file,const ColMeta& col){
    file.write(col.name,cns);
    writeBinary(file,col.type);
    writeBinary(file,col.size);
    writeBinary(file,col.offset);
    writeBinary(file,col.isPK);
}

void readColumn(istream& file,ColMeta& col){
    file.read(col.name,cns);
    readBinary(file,col.type);
    readBinary(file,col.size);
    readBinary(file,col.offset);
    readBinary(file,col.isPK);
}

uint64_t writeHeader(fstream& file,bool deleted){
    uint64_t timestamp =
        chrono::duration_cast<chrono::milliseconds>(
            chrono::system_clock::now().time_since_epoch()
        ).count();
    writeBinary(file,timestamp);
    writeBinary(file,deleted);
    return timestamp;
}

void writePayload(fstream& file,const TableMeta& meta,const Row& row){
    char temp[cns]={};
    for(int i=0;i<meta.columnCount;i++){
        if(meta.columns[i].type==INT){
            int x=stoi(row.values[i]);
            writeBinary(file,x);
        }
        else if(meta.columns[i].type==FLOAT){
            float x=stof(row.values[i]);
            writeBinary(file,x);
        }
        else if(meta.columns[i].type==BOOL){
            bool x=(row.values[i]=="true");
            writeBinary(file,x);
        }
        else if(meta.columns[i].type==STRING){
            memset(temp,0,cns);
            int len=row.values[i].size();
            if(len>meta.columns[i].size-1) len=meta.columns[i].size-1;
            memcpy(temp,row.values[i].data(),len);
            file.write(temp,meta.columns[i].size);
        }
    }
}

Row readPayload(fstream& file,const TableMeta& meta){
    Row row;
    for(int i=0;i<meta.columnCount;i++){
        if(meta.columns[i].type==INT){
            int x;
            readBinary(file,x);
            row.values.push_back(to_string(x));
        }
        else if(meta.columns[i].type==FLOAT){
            float x;
            readBinary(file,x);
            row.values.push_back(to_string(x));
        }
        else if(meta.columns[i].type==BOOL){
            bool x;
            readBinary(file,x);
            row.values.push_back(x?"true":"false");

        }
        else if(meta.columns[i].type==STRING){
            string temp(meta.columns[i].size,'\0');
            file.read(&temp[0], meta.columns[i].size);
            temp.resize(strnlen(temp.c_str(),meta.columns[i].size));
            row.values.push_back(temp);
        }
    }
    return row;
}