#include "history_index.h"

bool HistoryIndex::contains(const string& pk){ return history.count(pk); }

void HistoryIndex::addVersion(const string& pk,const RecordVersion& version){ history[pk].push_back(version); }

vector<RecordVersion>& HistoryIndex::getHistory(const string& pk){ return history.at(pk); }

const RecordVersion& HistoryIndex::latest(const string& pk){ return history.at(pk).back(); }

int HistoryIndex::size(){ return history.size(); }

const RecordVersion* HistoryIndex::latestBefore(const string& pk,uint64_t timestamp){
    auto it=history.find(pk);
    if(it==history.end()) return nullptr;
    
    const vector<RecordVersion>& hist=it->second;
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

int HistoryIndex::lowerBound(const string& pk,uint64_t timestamp){
    auto it=history.find(pk);
    if(it==history.end()) return -1;
    const vector<RecordVersion>& hist=it->second;

    int l=0,r=hist.size()-1,ans=-1;
    while(l<=r){
        int mid=l+(r-l)/2;
        if(hist[mid].timestamp>=timestamp){
            ans=mid;
            r=mid-1;
        }
        else l=mid+1;
    }
    return ans;
}

vector<string> HistoryIndex::list(){
    vector<string> ans;
    for(const auto& v:history) ans.push_back(v.first);
    return ans;
}