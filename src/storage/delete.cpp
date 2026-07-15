#include "table.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include "serializer.h"

bool Table::deleteRow(const string& pk) {
    auto it = history.find(pk);
    if (it == history.end()) {
        cerr << "No row found with primary key '" << pk << "'.\n"; 
        return false;
    }
    
    uint64_t latestOffset = it->second; 

    fstream file("data/" + string(meta.name) + ".db", ios::binary | ios::in | ios::out);
    if (!file) {
        cerr << "Failed to open table file '" << meta.name << "'.\n"; 
        return false;
    }
    
    file.seekg(latestOffset + sizeof(uint64_t), ios::beg);
    bool isDeleted;
    readBinary(file, isDeleted);
    
    if (isDeleted) {
        cerr << "Row with primary key '" << pk << "' is already deleted.\n"; 
        file.close();
        return false;
    }
    
    
    file.seekg(latestOffset + rhsz, ios::beg);
    vector<char> temp(meta.payloadSize);
    file.read(temp.data(), meta.payloadSize);
    
    
    file.seekp(0, ios::end);
    uint64_t newOffset = file.tellp();
    uint64_t timestamp = chrono::duration_cast<chrono::milliseconds>(
        chrono::system_clock::now().time_since_epoch()
    ).count();
    bool deletedFlag = true;

    
    writeBinary(file, timestamp);
    writeBinary(file, deletedFlag);
    writeBinary(file, latestOffset); 
    
    
    file.write(temp.data(), meta.payloadSize);
    
    
    it->second = newOffset;

    file.close();
    return true;
}