#pragma once
#include <fstream>
#include "types.h"
using namespace std;

template<typename T>
void writeBinary(ofstream& file,const T& value){
    file.write(reinterpret_cast<const char*>(&value),sizeof(T));
}

template<typename T>
void readBinary(ifstream& file,T& value){
    file.read(reinterpret_cast<char*>(&value),sizeof(T));
}

void writeColumn(ofstream& file,const ColMeta& col);
void readColumn(ifstream& file,ColMeta& col);