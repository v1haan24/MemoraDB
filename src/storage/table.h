#pragma once
#include <string>
#include <cstdint>
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

void print(const Row& row);
void print(const Record& record);
void print(const vector<Row>& rows);
void print(const vector<Record>& records);
void print(const Difference& diff);
void print(const vector<Difference>& diff);

vector<Difference> compareRecords(const Record& before,const Record& after,const TableMeta& meta);

class Table{
    TableMeta meta;
    HistoryIndex history;
    void recoverState();
    
    //misc
    bool validateRow(const Row& row);
    bool validateValue(const string& value,const ColMeta& col);
    string getPrimaryKey(const Row& row);
    bool appendRecord(const Record& record);
public:
    Table(const TableMeta& metadata){meta=metadata; recoverState();}
    bool insert(const Row& row);
    bool update(const Row& row);
    bool deleteRow(const string& pk);
    Record latest(const string& pk);
    Record readRecord(uint64_t offset);
    void printDatabase();
    TableMeta& getMeta(){ return meta;}

    //temporal
    Record selectAsOf(const string& pk,uint64_t timestamp);
    vector<Record> selectBetween(const string& pk,uint64_t t1,uint64_t t2);
    vector<Record> showHistory(const string& pk);
    vector<Record> snapshot(uint64_t timestamp);
    vector<Difference> compare(const string& pk,uint64_t t1,uint64_t t2);
    vector<Difference> evolution(const string& pk,uint64_t t1,uint64_t t2);
    bool rollback(const string& pk,uint64_t timestamp);
    bool rollback(uint64_t timestamp);
};