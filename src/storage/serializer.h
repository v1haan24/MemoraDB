#pragma once
#include <fstream>
#include "../types.h"
using namespace std;

template<typename T>
void writeBinary(ostream& file,const T& value){
    file.write(reinterpret_cast<const char*>(&value),sizeof(T));
}

template<typename T>
void readBinary(istream& file,T& value){
    file.read(reinterpret_cast<char*>(&value),sizeof(T));
}

void writeColumn(ostream& file,const ColMeta& col);
void readColumn(istream& file,ColMeta& col);
