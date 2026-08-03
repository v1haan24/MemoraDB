#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include "../common/metadata.h"

class HistoryIndex{
private:
    std::unordered_map<std::string,std::vector<RecordVersion>> history;
public:
    bool contains(const std::string& pk);
    void addVersion(const std::string& pk,const RecordVersion& version);
    const std::vector<RecordVersion>& getHistory(const std::string& pk);
    const RecordVersion& latest(const std::string& pk);
    int size();
    const RecordVersion* latestBefore(const std::string& pk,uint64_t timestamp);
    std::vector<std::string> list();
};