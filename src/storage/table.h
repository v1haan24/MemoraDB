#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include "../types.h"
using namespace std;

struct Row{
    vector<string> values; //assuming parser gives string as output
};

class Table{
    TableMeta meta;
    bool validateRow(const Row& row);
    bool validateValue(const string& value,const ColMeta& col);
    uint64_t writeHeader(fstream& file);
    void writePayload(fstream& file,const Row& row);
    Row readPayload(fstream& file);
    string getPrimaryKey(const Row& row);
    Row readRow(uint64_t offset);
    void recoverState();
public:
    unordered_map<string,vector<RecordVersion>> history; //making it public for testing
    Table(const TableMeta& metadata){
        meta=metadata;
        recoverState();
    }
    bool insert(const Row& row);
    Row latest(const string& pk);
};