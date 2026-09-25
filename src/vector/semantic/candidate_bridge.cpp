#include "candidate_bridge.h"

std::vector<VCandidate> refineCandidates(const std::vector<Record>& records,const TableMeta& meta){
    std::vector<VCandidate> candidates;
    candidates.reserve(records.size());
    int32_t pkIndex=-1;
    for(uint32_t i=0;i<meta.columnCount;i++){
        if(meta.columns[i].isPK){
            pkIndex=i;
            break;
        }
    }
    if(pkIndex==-1) return candidates;
    for(const auto& record:records){
        if(record.row.values.empty()) continue;
        if(record.deleted) continue;
        candidates.push_back({record.row.values[pkIndex],record.timestamp});
    }
    return candidates;
}
