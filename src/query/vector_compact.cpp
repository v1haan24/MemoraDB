#include "vector_compact.h"

bool compactTable(Table& table, vecTable& vt, VectorIndex& idx, uint64_t timestamp){
    if(!table.compact(timestamp, &vt, &idx)) return false;

    std::vector<std::string> deletedPks;
    for(const auto& pk : table.getPrimaryKeys()){
        if(table.latest(pk).deleted) deletedPks.push_back(pk);
    }
    if(!deletedPks.empty() && !vt.purge(deletedPks, timestamp)) return false;
    idx.buildIndex(vt);

    return true;
}