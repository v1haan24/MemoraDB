#pragma once
#include <string>
#include <iostream>
#include "../common/metadata.h"
#include "../index/history_index.h"
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
uint64_t writeHeader(fstream& file,bool deleted);
void writePayload(fstream& file,const TableMeta& meta,const Row& row);
Row readPayload(fstream& file,const TableMeta& meta);

class Table{
    TableMeta meta;
    HistoryIndex history;
    string getPrimaryKey(const Row& row);
    void recoverState();
    bool validateRow(const Row& row);
    bool validateValue(const string& value,const ColMeta& col);
public:
    Table(const TableMeta& metadata){meta=metadata; recoverState();}
    bool insert(const Row& row);
    bool update(const Row& row);
    bool deleteRow(const string& pk);
    Row readRow(uint64_t offset);
    Row latest(const string& pk);
    void printDatabase();
    TableMeta& getMeta(){ return meta;}
};