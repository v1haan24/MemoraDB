#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include "../common/metadata.h"
using namespace std;

class HistoryIndex{
private:
    unordered_map<string,vector<RecordVersion>> history;
public:
    bool contains(const string& pk);
    void addVersion(const string& pk,const RecordVersion& version);
    vector<RecordVersion>& getHistory(const string& pk);
    const RecordVersion& latest(const string& pk);
    int size();
    const RecordVersion* latestBefore(const string& pk,uint64_t timestamp);
    int lowerBound(const string& pk,uint64_t timestamp);
    vector<string> list();
};