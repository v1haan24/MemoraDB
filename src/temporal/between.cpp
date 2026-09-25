#include <cstddef>
#include "../storage/table.h"

std::vector<Record> Table::selectBetween(const std::string& pk,uint64_t t1,uint64_t t2){
    if(t1>t2){ std::cerr<<"Invalid timestamp range.\n"; return {};}
    std::vector<Record> ans;
    if(!history.contains(pk)){ std::cerr<<"No row found.\n"; return {};}
    const std::vector<RecordVersion>& hist=history.getHistory(pk);
    const RecordVersion* start=history.latestBefore(pk,t1);
    std::ptrdiff_t idx = (start != nullptr) ? start - hist.data() : 0;
    if(idx >= static_cast<std::ptrdiff_t>(hist.size()) || hist[idx].timestamp > t2) return {};
    for(std::ptrdiff_t i=idx;i<static_cast<std::ptrdiff_t>(hist.size());i++){
        if(hist[i].timestamp>t2) break;
        ans.push_back(readRecord(hist[i].offset));
    }
    return ans;
}
std::vector<VCandidate> Table::selectBetweenKeys(const std::string& pk,uint64_t t1,uint64_t t2){
    if(t1>t2){ std::cerr<<"Invalid timestamp range.\n"; return {};}
    std::vector<VCandidate> ans;
    if(!history.contains(pk)){ std::cerr<<"No row found.\n"; return {};}
    const std::vector<RecordVersion>& hist=history.getHistory(pk);
    const RecordVersion* start=history.latestBefore(pk,t1);
    std::ptrdiff_t idx = (start != nullptr) ? start - hist.data() : 0;
    if(idx >= static_cast<std::ptrdiff_t>(hist.size()) || hist[idx].timestamp > t2) return {};
    for(std::ptrdiff_t i=idx;i<static_cast<std::ptrdiff_t>(hist.size());i++){
        if(hist[i].timestamp>t2) break;
        if(isDeleted(hist[i].offset)) continue;
        ans.push_back({pk,hist[i].timestamp});
    }
    return ans;
}