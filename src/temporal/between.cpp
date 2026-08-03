#include "../storage/table.h"

std::vector<Record> Table::selectBetween(const std::string& pk,uint64_t t1,uint64_t t2){
    std::vector<Record> ans;
    if(!history.contains(pk)){ std::cerr<<"No row found.\n"; return {};}
    const std::vector<RecordVersion>& hist=history.getHistory(pk);
    const RecordVersion* start=history.latestBefore(pk,t1);
    if(start==nullptr) return {};
    int idx=start-hist.data();
    for(int i=idx;i<hist.size();i++){
        if(hist[i].timestamp>t2) break;
        ans.push_back(readRecord(hist[i].offset));
    }
    return ans;
}