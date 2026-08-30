#include "../storage/table.h"

Record Table::selectAsOf(const std::string& pk,uint64_t timestamp){
    const RecordVersion* version=history.latestBefore(pk,timestamp);
    
    if(version==nullptr){
        std::cerr<<"No version exists before given timestamp.\n";
        return {};
    }
    return readRecord(version->offset);
}

VCandidate Table::selectAsOfKeys(const std::string& pk,uint64_t timestamp){
    const RecordVersion* version=history.latestBefore(pk,timestamp);

    if(version==nullptr){
        std::cerr<<"No version exists before given timestamp.\n";
        return {};
    }
    return {pk,version->timestamp};
}