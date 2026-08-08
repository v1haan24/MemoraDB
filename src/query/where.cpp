#include "query.h"

bool evaluate(const std::string& lhs,const ColMeta& col,const WhereClause& clause){
    const std::string rhs=clause.value;
    if(col.type==INT){
        int a=std::stoi(lhs);
        int b=std::stoi(rhs);
        switch(clause.op){
                case EQ: return a==b;
                case NE: return a!=b;
                case LT: return a<b;
                case LE: return a<=b;
                case GT: return a>b;
                case GE: return a>=b;
        }
    }
    else if(col.type==FLOAT){
        float a=std::stof(lhs);
        float b=std::stof(rhs);
        switch(clause.op){
                case EQ: return a==b;
                case NE: return a!=b;
                case LT: return a<b;
                case LE: return a<=b;
                case GT: return a>b;
                case GE: return a>=b;
        }
    }
    else if(col.type==STRING){
        switch(clause.op){
                case EQ: return lhs==rhs;
                case NE: return lhs!=rhs;
                case LT: return lhs<rhs;
                case LE: return lhs<=rhs;
                case GT: return lhs>rhs;
                case GE: return lhs>=rhs;
        }
    }
    else if(col.type==BOOL){
         bool a=(lhs=="true");
         bool b=(rhs=="true");
        switch(clause.op){
                case EQ: return a==b;
                case NE: return a!=b;
                case LT:
                case LE:
                case GT:
                case GE:
                    return false;
        }
    }
    return false;
}

std::vector<Record> where(const std::vector<Record>& candidates,const TableMeta& meta,const WhereClause& clause){
    std::vector<Record> ans={};
    for(const auto& candidate:candidates){
        const ColMeta& column=meta.columns[clause.column];
        const std::string& value=candidate.row.values[clause.column];
        if(evaluate(value,column,clause)==true) ans.push_back(candidate);
    }
    return ans;
}