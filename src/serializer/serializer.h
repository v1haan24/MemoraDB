#pragma once
#include <fstream>
#include <cstdint>
#include "../common/metadata.h"
#include "../common/row.h"
using namespace std;

template<typename T>
void writeBinary(ostream& file,const T& value){
    file.write(reinterpret_cast<const char*>(&value),sizeof(T));
}
template<typename T>
void readBinary(istream& file,T& value){
    file.read(reinterpret_cast<char*>(&value), sizeof(T));
}

class Serializer{
public:
    void writeColumn(ostream& file,const ColMeta& col);
    void readColumn(istream& file,ColMeta& col);
    uint64_t writeHeader(fstream& file,bool deleted);
    void writePayload(fstream& file,const TableMeta& meta,const Row& row);
    Row readPayload(fstream& file,const TableMeta& meta);
};