#include "vecTable.h"
#include <iostream>

vecTable::vecTable(const VectorMeta& metadata){
    meta = metadata;
}

bool vecTable::insert(std::string pk, uint64_t timestamp, std::vector<float> embed){
    std::fstream file(meta.tablePath/"table.vec" ,std::ios::binary|std::ios::in|std::ios::out);
    if(!file)
    {
        std::cerr<<"ERROR: Couldn't open file!!"<<std::endl;
        return false;
    }

    file.clear();
    file.seekp(0,std::ios::end);
    writeBinary(file, meta.rowCount); //rowcount will act as an int id for each record
    meta.rowCount++;

    file.write(pk.c_str(), pk.size());
    writeBinary(file, timestamp);
    writeBinary(file, embed.data());

    file.seekp(sizeof(int)+tns, std::ios::beg);
    writeBinary(file,meta.rowCount);
    file.close();

}