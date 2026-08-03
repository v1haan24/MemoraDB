#include "../storage/table.h"


std::vector<Record> Table::snapshot(uint64_t timestamp){
    std::vector<Record> ans;
    const std::vector<std::string> keys=history.list();
    ans.reserve(keys.size());

    for(const auto& pk:keys){
        Record record=selectAsOf(pk,timestamp);
        if(record.row.values.empty()) continue;
        if(!record.deleted) ans.push_back(record);
    }
    return ans;
}