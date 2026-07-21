#include "../storage/table.h"
using namespace std;

vector<Record> Table::snapshot(uint64_t timestamp){
    vector<Record> ans;
    const vector<string> keys=history.list();
    ans.reserve(keys.size());

    for(const auto& pk:keys){
        Record record=selectAsOf(pk,timestamp);
        if(!record.deleted) ans.push_back(record);
    }
    return ans;
}