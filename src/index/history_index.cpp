#include "history_index.h"

bool HistoryIndex::contains(const std::string& pk){ return history.count(pk); }

void HistoryIndex::addVersion(const std::string& pk,const RecordVersion& version){ history[pk].push_back(version); }

const std::vector<RecordVersion>& HistoryIndex::getHistory(const std::string& pk){ return history.at(pk); }

const RecordVersion& HistoryIndex::latest(const std::string& pk){ return history.at(pk).back(); }

int HistoryIndex::size(){ return history.size(); }

std::vector<std::string> HistoryIndex::list(){
    std::vector<std::string> ans;
    for(const auto& v:history) ans.push_back(v.first);
    return ans;
}

const RecordVersion* HistoryIndex::latestBefore(const std::string& pk,uint64_t timestamp){
    auto it=history.find(pk);
    if(it==history.end()) return nullptr;
    
    const std::vector<RecordVersion>& hist=it->second;
    const RecordVersion* ans=nullptr;
    int l=0,r=hist.size()-1;
    while(l<=r){
        int mid=l+(r-l)/2;
        if(hist[mid].timestamp<=timestamp){
            ans=&hist[mid];
            l=mid+1;
        }
        else r=mid-1;
    }
    return ans;
}