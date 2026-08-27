#include "../storage/table.h"

std::vector<Record> Table::showHistory(const std::string& pk){
    if(!history.contains(pk)){ std::cerr<<"No row found.\n"; return {};}
    const std::vector<RecordVersion>& hist=history.getHistory(pk);
    std::vector<Record> ans; ans.reserve(hist.size());
    for(const auto& version:hist) ans.push_back(readRecord(version.offset));
    return ans;
}

std::vector<VCandidate> Table::showHistoryKeys(const std::string& pk){
    if(!history.contains(pk)){ std::cerr<<"No row found.\n"; return {};}
    const std::vector<RecordVersion>& hist=history.getHistory(pk);
    std::vector<VCandidate> ans; ans.reserve(hist.size());
    for(const auto& version:hist){
        if(isDeleted(version.offset)) continue;
        ans.push_back({pk,version.timestamp});
    }
    return ans;
}