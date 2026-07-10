#include "serializer.h"
#include "table.h"
#include <chrono>

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

uint64_t Table::writeHeader(fstream& file){
    uint64_t timestamp =
        chrono::duration_cast<chrono::milliseconds>(
            chrono::system_clock::now().time_since_epoch()
        ).count();
    bool deleted=false;
    writeBinary(file,timestamp);
    writeBinary(file,deleted);
    return timestamp;
}

void Table::writePayload(fstream& file,const Row& row){
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
            vector<char> temp(meta.columns[i].size,'\0');
            memcpy(temp.data(),
            row.values[i].data(),
            min((int)row.values[i].size(),meta.columns[i].size-1));
            file.write(temp.data(),meta.columns[i].size);
        }
    }
}

