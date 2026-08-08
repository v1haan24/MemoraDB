#include "query.h"

std::vector<Record> generateCandidates(Table& table,CandidateMode mode,uint64_t t1,uint64_t t2){
    std::vector<Record> candidates;
    if(mode==LATEST) return table.scanLatest();
    else if(mode==SNAPSHOT) return table.snapshot(t1);
    else if(mode==BETWEEN){
        auto keys=table.getPrimaryKeys();
        for(const auto& pk:keys){
            auto records=table.selectBetween(pk,t1,t2);
            candidates.insert(candidates.end(),records.begin(),records.end());
        }
        return candidates;
    }
    else if(mode==HISTORY){
        auto keys=table.getPrimaryKeys();
        for(const auto& pk:keys){
            auto records=table.showHistory(pk);
            candidates.insert(candidates.end(),records.begin(),records.end());
        }
        return candidates;
    }
    return {};
}