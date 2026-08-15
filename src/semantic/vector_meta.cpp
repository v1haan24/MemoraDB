#include "vector_meta.h"
#include <iostream>
#include <filesystem>
#include <cstring>

bool vecMeta::createVecTable(VectorMeta& vec, TableMeta& table){
    vec.tablePath="data/"+std::string(table.name)+"/" +std::string(table.name)+ ".vec";

    if(std::filesystem::exists(vec.tablePath)){
        std::cerr<<"Vector table for '"<<table.name<<"' already exists.\n";
        return false;
    }

    std::filesystem::create_directories("data/"+std::string(table.name));

    int pkSize=0, pkCount=0;
    for(auto &c:table.columns){
        if(c.isPK){ pkSize+=c.size; pkCount++; }
    }
    if(pkCount!=1){ std::cerr<<"Table '"<<table.name<<"' must have exactly one primary key to create a vector table.\n"; return false; }
    if(pkSize<=0){ std::cerr<<"Invalid primary key size for '"<<table.name<<"'.\n"; return false; }

    std::ofstream file(vec.tablePath, std::ios::binary);
    if(!file){ std::cerr<<"Failed to create file "<<vec.tablePath<<" for table "<<table.name<<"\n"; return false; }

    vec.pkSize = pkSize;
    vec.recordCount = 0;
    vec.payloadSize = sizeof(uint32_t) + vec.pkSize + sizeof(uint64_t) + sizeof(float)*VEC_DIM; //id, pk, timestamp, vector
    vec.metadataSize = sizeof(int) + tns + sizeof(int) + sizeof(uint32_t) + sizeof(int); //metadataSize, name, pkSize, recordCount, payloadSize 

    std::string nam = "Vector_DB_"+std::string(table.name);
    strncpy(vec.name, nam.c_str(), tns-1);
    vec.name[tns-1] = '\0';

    writeBinary(file, vec.metadataSize);
    file.write(vec.name,tns);
    writeBinary(file, vec.pkSize);
    writeBinary(file,vec.recordCount);
    writeBinary(file,vec.payloadSize);

    if(!file){ std::cerr<<"Failed to write header for "<<vec.tablePath<<"\n"; return false; }

    file.close();
    return true;
}

VectorMeta vecMeta::readMetadata(const std::string& fileName){
    std::filesystem::path filePath(fileName);
    std::string tableName = filePath.stem().string();
    std::filesystem::path path = std::filesystem::path("data") / tableName / fileName;

    std::ifstream file(path, std::ios::binary);
    if(!file){
        std::cerr<<"Unable to open vector metadata file "<<fileName<<"\n";
        return {};
    }

    VectorMeta vec;
    readBinary(file, vec.metadataSize);
    file.read(vec.name, tns);
    readBinary(file, vec.pkSize);
    readBinary(file, vec.recordCount);
    readBinary(file, vec.payloadSize);

    if(!file){
        std::cerr<<"Failed to read vector metadata from "<<fileName<<"\n";
        return {};
    }

    vec.tablePath = path;
    return vec;
}
