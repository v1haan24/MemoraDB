#include "../storage/table.h"

std::vector<Difference> compareRecords(const Record& r1,const Record& r2,const TableMeta& meta){
    std::vector<Difference> ans;
    if(r1.deleted!=r2.deleted){
        Difference diff;
        diff.column="Deleted";
        diff.before=r1.deleted?"true":"false";
        diff.after=r2.deleted?"true":"false";
        ans.push_back(diff);
    }
    for(int i=0;i<meta.columnCount;i++){
        if(r1.row.values[i]!=r2.row.values[i]){
            Difference diff;
            diff.column=meta.columns[i].name;
            diff.before=r1.row.values[i];
            diff.after=r2.row.values[i];
            ans.push_back(diff);
        }
    }
    return ans;
}
std::vector<Difference> Table::compare(const std::string& pk,uint64_t t1,uint64_t t2){
    if(t1>t2){ std::cerr<<"Invalid time range.\n"; return {};}
    std::vector<Record> recs=selectBetween(pk,t1,t2);
    if(recs.empty()) return {};
    return compareRecords(recs.front(),recs.back(),meta);
}
std::vector<Difference> Table::evolution(const std::string& pk,uint64_t t1,uint64_t t2){
    if(t1>t2){ std::cerr<<"Invalid time range.\n"; return {};}
    std::vector<Difference> ans;
    if(!history.contains(pk)){ std::cerr<<"No row found.\n"; return {};}
    const std::vector<RecordVersion>& hist=history.getHistory(pk);
    const RecordVersion* start=history.latestBefore(pk,t1);
    int idx = (start != nullptr) ? static_cast<int>(start - hist.data()) : 0;
    if(idx >= static_cast<int>(hist.size()) || hist[idx].timestamp > t2) return ans;
    while(idx+1<hist.size() && hist[idx+1].timestamp<=t2){
        Record r1=readRecord(hist[idx].offset);
        Record r2=readRecord(hist[idx+1].offset);
        std::vector<Difference> changes=compareRecords(r1,r2,meta);
        for(auto& diff:changes){
            diff.timestamp=r2.timestamp;
            ans.push_back(diff);
        }
        idx++;
    }
    return ans;
}