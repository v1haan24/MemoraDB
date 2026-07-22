#include "table.h"
#include <iostream>
#include <fstream>
using namespace std;

bool Table::insert(const Row& row){
    if(!validateRow(row)) return false;

    string pk=getPrimaryKey(row);
    if(history.contains(pk)){cerr<<"Primary key '"<<pk<<"' already exists.\n"; return false;}

    fstream file("data/"+string(meta.name)+".db",ios::binary|ios::in|ios::out);
    if(!file){ cerr<<"Failed to open table file '"<<meta.name<<"'.\n"; return false; }

    file.seekp(0,ios::end); 
    uint64_t offset=file.tellp();
    uint64_t t=writeHeader(file,false);
    writePayload(file,meta,row);
    if(!file){
        cerr<<"Failed to write row to disk.\n";
        file.close();
        return false;
    }
    history.addVersion(pk,{t, offset});
    //meta.rowCount++;
    //file.seekp(sizeof(int)+tns,ios::beg);
    //writeBinary(file,meta.rowCount);
    file.close();
    return true;
}