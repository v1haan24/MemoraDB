#pragma once
#include "../storage/table.h"

enum Operator{EQ,NE,LT,LE,GT,GE};

enum CandidateMode{LATEST,SNAPSHOT,BETWEEN,HISTORY};

struct WhereClause{
    int column;
    Operator op;
    std::string value;
};
std::vector<Record> generateCandidates(Table& table,CandidateMode mode,uint64_t t1=0,uint64_t t2=0);
bool evaluate(const std::string& lhs,const ColMeta& col,const WhereClause& where);
std::vector<Record> where(const std::vector<Record>& candidates,const TableMeta& meta,const WhereClause& where);
std::vector<Record> sortRecords(const std::vector<Record>& records,const TableMeta& meta,int column,bool ascending=true);
std::vector<Record> limitRecords(const std::vector<Record>& records,int limit);
Record project(const Record& record,const std::vector<int>& columns);
std::vector<Record> project(const std::vector<Record>& records,const std::vector<int>& columns);
int count(const std::vector<Record>& records){return records.size();}