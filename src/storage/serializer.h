#pragma once
#include <fstream>
#include "../types.h"

// standard binary write/read using reinterpret_cast
template<typename T>
void writeBinary(std::ofstream& file, const T& value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template<typename T>
void readBinary(std::ifstream& file, T& value) {
    file.read(reinterpret_cast<char*>(&value), sizeof(T));
}

void writeColumnInfo(std::ofstream& file, const ColumnInfo& col);
void readColumnInfo(std::ifstream& file, ColumnInfo& col);