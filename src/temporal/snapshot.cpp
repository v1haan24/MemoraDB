#include "../storage/table.h"


std::vector<Record> Table::snapshot(uint64_t timestamp){
    std::vector<Record> ans;
    const std::vector<std::string> keys=history.list();
    ans.reserve(keys.size());

    for(const auto& pk:keys){
        Record record=selectAsOf(pk,timestamp);
        if(record.row.values.empty()) continue;
        if(!record.deleted) ans.push_back(record);
    }
    return ans;
}

std::vector<VCandidate> Table::snapshotKeys(uint64_t timestamp){
    std::vector<VCandidate> keys;
    const std::vector<std::string> pks=history.list();
    keys.reserve(pks.size());
    for(const auto& pk:pks){
        const RecordVersion* version=history.latestBefore(pk,timestamp);
        if(version==nullptr) continue;
        if(isDeleted(version->offset)) continue;
        keys.push_back({pk,version->timestamp});
    }
    return keys;
}