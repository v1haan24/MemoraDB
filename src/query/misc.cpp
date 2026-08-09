#include "query.h"
#include <algorithm>

int count(const std::vector<Record>& records){return records.size();}

std::vector<Record> sortRecords(const std::vector<Record>& records,const TableMeta& meta,int column,bool ascending){
    if(column<0 || column>=meta.columnCount) return {};
    std::vector<Record> ans=records;
    const ColMeta& col=meta.columns[column];
    std::sort(ans.begin(),ans.end(),
        [&](const Record& a,const Record& b){
            if(col.type==INT){
                int x=std::stoi(a.row.values[column]);
                int y=std::stoi(b.row.values[column]);
                return ascending?x<y:x>y;
            }
            if(col.type==FLOAT){
                float x=std::stof(a.row.values[column]);
                float y=std::stof(b.row.values[column]);
                return ascending?x<y:x>y;
            }
            return ascending? a.row.values[column]<b.row.values[column]: a.row.values[column]>b.row.values[column];
        }
    );
    return ans;
}

std::vector<Record> limitRecords(const std::vector<Record>& records,int limit){
    if(limit<0) return {};
    if(limit>=records.size()) return records;
    return std::vector<Record>(records.begin(),records.begin()+limit);
}