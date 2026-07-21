#include "../storage/table.h"
using namespace std;

Record Table::selectAsOf(const string& pk,uint64_t timestamp){
    const RecordVersion* version=history.latestBefore(pk,timestamp);
    
    if(version==nullptr){
        cerr<<"No version exists before given timestamp.\n";
        return {};
    }
    return readRecord(version->offset);
}