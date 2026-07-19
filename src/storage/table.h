#pragma once
#include <string>
#include <iostream>
#include "../common/metadata.h"
#include "../common/row.h"
#include "../index/history_index.h"
#include "../validator/validator.h"
#include "../serializer/serializer.h"
#include "../recovery/recovery.h"
using namespace std;

class Table{
    TableMeta meta;
    HistoryIndex history;
    Validator validator;
    Serializer serializer;
    Recovery recovery;
    string getPrimaryKey(const Row& row){
        for(int i=0;i<meta.columnCount;i++){
            if(meta.columns[i].isPK) return row.values[i];
        }
        cerr<<"Primary key not found.\n";
        return "";
    }
public:
    Table(const TableMeta& metadata){meta=metadata; recovery.recoverState(meta,history);}
    bool insert(const Row& row);
    bool update(const Row& row);
    bool deleteRow(const string& pk);
    Row readRow(uint64_t offset);
    Row latest(const string& pk);
    void printDatabase();
    TableMeta& getMeta(){ return meta;}
};