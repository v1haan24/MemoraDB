#include "vector_meta.h"
#include <iostream>
#include <filesystem>
#include <cstring>

bool vecMeta::createVecTable(VectorMeta& vec, TableMeta& table){
    vec.tablePath="data/"+std::string(table.name);
    
    std::ofstream file(vec.tablePath/"table.vec",std::ios::binary);
    if(!file){std::cerr<<"Failed to create file table.vec for "<<vec.tablePath<<"\n"; return false;}

    vec.metadataSize = sizeof(uint64_t) + tns + sizeof(int) + sizeof(int);
    std::string nam = "Vector_DB_"+std::string(table.name);
    strncpy(vec.name, nam.c_str(), tns-1);
    vec.name[tns-1] = '\0';

    writeBinary(file, vec.metadataSize);
    file.write(vec.name,tns);
    writeBinary(file,vec.rowCount);
    writeBinary(file,vec.payloadSize);
    
    file.close();
    return true;

}