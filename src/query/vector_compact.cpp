#include "vector_compact.h"

bool compactTable(Table& table, vecTable& vt, uint64_t timestamp){
    if(!table.compact(timestamp)) return false;

    std::vector<std::string> deletedPks;
    for(const auto& pk : table.getPrimaryKeys()){
        if(table.latest(pk).deleted) deletedPks.push_back(pk);
    }

    if(!vt.compact(timestamp)) return false;
    if(!deletedPks.empty() && !vt.purge(deletedPks, timestamp)) return false;

    return true;
}