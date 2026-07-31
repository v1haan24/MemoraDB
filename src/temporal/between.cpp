#include "../storage/table.h"
using namespace std;

vector<Record> Table::selectBetween(const string& pk,uint64_t t1,uint64_t t2){
    vector<Record> ans;
    if(!history.contains(pk)){ cerr<<"No row found.\n"; return {};}
    const vector<RecordVersion>& hist=history.getHistory(pk);
    const RecordVersion* start=history.latestBefore(pk,t1);
    if(start==nullptr) return {};
    int idx=0;
    while(idx<hist.size() && hist[idx].timestamp!=start->timestamp) idx++;
    if(idx==-1) return ans;
    for(int i=idx;i<hist.size();i++){
        if(hist[i].timestamp>t2) break;
        ans.push_back(readRecord(hist[i].offset));
    }
    return ans;
}