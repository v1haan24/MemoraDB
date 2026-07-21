#include "../storage/table.h"
using namespace std;

vector<Record> Table::selectBetween(const string& pk,uint64_t t1,uint64_t t2){
    vector<Record> ans;
    const vector<RecordVersion>& hist=history.getHistory(pk);
    int idx=history.lowerBound(pk,t1);
    if(idx==-1) return ans;
    for(int i=idx;i<hist.size();i++){
        if(hist[i].timestamp>t2) break;
        ans.push_back(readRecord(hist[i].offset));
    }
    return ans;
}