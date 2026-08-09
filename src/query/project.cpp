#include "query.h"

Record project(const Record& record,const std::vector<int>& columns){
    Record result=record;
    result.row.values.clear();
    for(int column:columns) result.row.values.push_back(record.row.values[column]);
    return result;
}

std::vector<Record> project(const std::vector<Record>& records,const std::vector<int>& columns){
    std::vector<Record> ans;
    ans.reserve(records.size());
    for(const auto& record:records) ans.push_back(project(record,columns));
    return ans;
}