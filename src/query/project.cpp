#include "query.h"

Record project(const Record& record,const std::vector<int32_t>& columns){
    Record result=record;
    result.row.values.clear();
    for(int32_t column:columns){
        if(column<0 || column>=record.row.values.size()){
            std::cerr<<"Invalid column index.\n";
            return {};
        }
        result.row.values.push_back(record.row.values[column]);
    }
    return result;
}

std::vector<Record> project(const std::vector<Record>& records,const std::vector<int32_t>& columns){
    std::vector<Record> ans;
    ans.reserve(records.size());
    for(const auto& record:records) ans.push_back(project(record,columns));
    return ans;
}