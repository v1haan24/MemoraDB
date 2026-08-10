#include "table.h"

void Table::openFile(){
    file.open(filePath,std::ios::binary|std::ios::in|std::ios::out);
    if(!file.is_open()){
        file.clear();
        file.open(filePath,std::ios::binary|std::ios::out);
        file.close();
        file.open(filePath,std::ios::binary|std::ios::in|std::ios::out);
    }
}

Table::Table(const TableMeta& metadata){
    meta=metadata;
    filePath="data/"+std::string(meta.name)+"/data.db";
    openFile();
    recoverState();
}
