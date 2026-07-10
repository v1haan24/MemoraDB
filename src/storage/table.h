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
    unordered_map<string,vector<RecordVersion>> history;
    bool validateRow(const Row& row);
    bool validateValue(const string& value,const ColMeta& col);
    uint64_t writeHeader(fstream& file);
    void writePayload(fstream& file,const Row& row);
    string getPrimaryKey(const Row& row);
public:
    Table(const TableMeta& metadata){
        meta=metadata;
    }
    bool insert(const Row& row);
};