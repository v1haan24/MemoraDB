#include "table.h"
#include <climits>
#include <cctype>

string Table::getPrimaryKey(const Row& row){
    string pk;
    for(int i=0;i<meta.columnCount;i++){
        if(meta.columns[i].isPK){ pk=row.values[i]; break;}
    }
    return pk;
}

bool Table::validateRow(const Row& row){
    if(row.values.size()!=meta.columnCount) return false;
    for(int i=0;i<meta.columnCount;i++){
        if(!validateValue(row.values[i],meta.columns[i])) return false;
    }
    return true;
}

bool isBool(const string& s){
    return s=="true" || s=="false";
}
bool isString(const string& s,int maxSize){
    return s.length()<=maxSize;
}
bool isInt(const string& s){
    if(s.length()==0) return false;
    int start=0;
    long long ans=0;
    if(s[0]=='-' || s[0]=='+') start=1;
    if(start>=s.length()) return false;
    for(int i=start;i<s.length();i++){
        if(!isdigit(s[i])) return false;
        ans=ans*10+(s[i]-'0');
        if((s[0]=='-' && -ans<INT_MIN) || (s[0]!='-' && ans>INT_MAX)) return false;
    }
    return true;
}
bool isFloat(const string& s){
    if(s.length()==0) return false;
    int start=0;
    bool dot=false,digit=false;
    if(s[0]=='-' || s[0]=='+') start=1;
    if(start>=s.length()) return false;
    for(int i=start;i<s.length();i++){
        if(isdigit(s[i])) digit=true;
        else if(s[i]=='.'){if(dot) return false; dot=true;}
        else return false;
    }
    return digit;
}

bool Table::validateValue(const string& value,const ColMeta& col){
    switch(col.type){
        case INT:
            return isInt(value);
        case FLOAT:
            return isFloat(value);
        case STRING:
            return isString(value,col.size);
        case BOOL:
            return isBool(value);
    }
    return false;
}