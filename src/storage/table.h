#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include "../types.h"
using namespace std;

struct Row
{
    vector<string> values; //Assuming parser will give the the values to be inserted as strings
};

class Table
{
    TableMeta meta;

    bool validateRow(const Row& row); //It will validate and verify if the row to be inserted is valid or not
    bool validateValue(const string& value, const ColMeta& col); //It will be used in validateRow to check if individual value is valid or not.

    uint64_t writeHeader(fstream& file, bool deleted=false); //It will write the header of row in the .db file before writing actual row and return timestamp

    void writePayload(fstream& file,const Row& row); //It will serialize & write the actual row values after the header in the .db file
    Row readPayload(fstream& file); //It will deserialize a particular row values and return it as a Row. This reading will be done manually used file object

    string getPrimaryKey(const Row& row); //Find the primary key in the inputed row values
    Row readRow(uint64_t offset); //Used to read entire row using in-memory hashmap offset

    void recoverState(); //TBD

    public:
    unordered_map<string,vector<RecordVersion>> history; //In-memory hashmap for indexing purpose. making it public for testing
    Table(const TableMeta& metadata){
        meta=metadata;
        recoverState(); //TBD
    }
    bool insert(const Row& row);
    bool Delete(const string& pk);
    //Row latest(const string& pk); => TBD

};