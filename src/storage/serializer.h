#pragma once
#include <fstream>
#include "../types.h"
using namespace std;

template<typename T>
void writeBinary(ostream& file,const T& value)  //It will be used in writing value in binary format to .db file i.e. in serialization
{
    file.write(reinterpret_cast<const char*>(&value),sizeof(T));
}

template<typename T>
void readBinary(istream& file,T& value) //It will be used in reading values in binary format from .db file to their actual datatype i.e. in deserialization
{
    file.read(reinterpret_cast<char*>(&value),sizeof(T));
}

void writeColumn(ostream& file,const ColMeta& col); //Used to write an individual column metadata to .db file
void readColumn(istream& file,ColMeta& col); //Used to read an individual column metadata from .db file