#pragma once
#include <string>
#include <cstdint>
#include <iostream>
#include <fstream>
#include "../common/metadata.h"
#include "../index/history_index.h"

template<typename T>
void writeBinary(std::ostream& file,const T& value){
    file.write(reinterpret_cast<const char*>(&value),sizeof(T));
}

template<typename T>
void readBinary(std::istream& file,T& value){
    file.read(reinterpret_cast<char*>(&value),sizeof(T));
}

bool writeMetadata(std::fstream& out,const TableMeta& meta);
void writeColumn(std::ostream& file,const ColMeta& col);
void readColumn(std::istream& file,ColMeta& col);
uint64_t writeHeader(std::fstream& file,bool deleted);
void writePayload(std::fstream& file,const TableMeta& meta,const Row& row);
Row readPayload(std::fstream& file,const TableMeta& meta);

bool validateValue(const std::string& value,const ColMeta& col);
std::vector<Difference> compareRecords(const Record& before,const Record& after,const TableMeta& meta);

void print(const Row& row);
void print(const Record& record);
void print(const std::vector<Row>& rows);
void print(const std::vector<Record>& records);
void print(const Difference& diff);
void print(const std::vector<Difference>& diff);


class Table{
    //metadata
    TableMeta meta;
    HistoryIndex history;

    //helpers
    void recoverState();
    void openFile();

    //file
    std::fstream file;
    std::string filePath;

    //misc
    std::string getPrimaryKey(const Row& row);
    bool validateRow(const Row& row);
    bool appendRecord(const Record& record);
public:
    Table(const TableMeta& metadata);
    
    //storage
    bool insert(const Row& row);
    bool update(const Row& row);
    bool deleteRow(const std::string& pk);
    bool compact(uint64_t timestamp);
    Record latest(const std::string& pk);
    Record readRecord(uint64_t offset);
    void printDatabase();
    TableMeta& getMeta(){ return meta;}

    //temporal
    Record selectAsOf(const std::string& pk,uint64_t timestamp);
    std::vector<Record> selectBetween(const std::string& pk,uint64_t t1,uint64_t t2);
    std::vector<Record> showHistory(const std::string& pk);
    std::vector<Record> snapshot(uint64_t timestamp);
    std::vector<Difference> compare(const std::string& pk,uint64_t t1,uint64_t t2);
    std::vector<Difference> evolution(const std::string& pk,uint64_t t1,uint64_t t2);
    bool rollback(const std::string& pk,uint64_t timestamp); //row roll-back
    bool rollback(uint64_t timestamp); //table roll-back

    //misc
    std::vector<std::string> getPrimaryKeys();
    Record latest(const std::string& pk);
    std::vector<Record> scanLatest();
    Record readRecord(uint64_t offset);
    void printDatabase();
    TableMeta& getMeta(){ return meta;}
};