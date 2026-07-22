#include "../storage/table.h"
using namespace std;

vector<Difference> compareRecords(const Record& r1,const Record& r2,const TableMeta& meta){
    vector<Difference> ans;
    if(r1.deleted!=r2.deleted){
        Difference diff;
        diff.column="Deleted";
        diff.before=r1.deleted?"true":"false";
        diff.after=r2.deleted?"true":"false";
        ans.push_back(diff);
    }
    for(int i=0;i<meta.columnCount;i++){
        if(r1.row.values[i]!=r2.row.values[i]) {
            Difference diff;
            diff.column=meta.columns[i].name;
            diff.before=r1.row.values[i];
            diff.after=r2.row.values[i];
            ans.push_back(diff);
        }
    }
    return ans;
}

vector<Difference> Table::compare(const string& pk,uint64_t t1,uint64_t t2){
    Record r1=selectAsOf(pk,t1), r2=selectAsOf(pk,t2);
    return compareRecords(r1,r2,meta);
}

vector<Difference> Table::evolution(const string& pk,uint64_t t1,uint64_t t2){
    vector<Difference> ans;
    const vector<RecordVersion>& hist=history.getHistory(pk);
    int idx=history.lowerBound(pk,t1);
    if(idx>0 && hist[idx].timestamp>t1) idx--;
    while(idx+1<hist.size() && hist[idx+1].timestamp<=t2){
        Record r1=readRecord(hist[idx].offset);
        Record r2=readRecord(hist[idx+1].offset);
        vector<Difference> changes=compareRecords(r1,r2,meta);
        for(auto& diff:changes){
            diff.timestamp=r2.timestamp;
            ans.push_back(diff);
        }
        idx++;
    }
    return ans;
}