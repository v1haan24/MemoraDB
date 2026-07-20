#include "table.h"
#include <sstream>

bool isBool(const string& s){
    return s=="true" || s=="false";
}

bool isString(const string& s,int maxSize){
    return s.length()<=maxSize;
}

bool isInt(const string& s){
    if(s.empty()) return false;
    stringstream ss(s);
    int d;
    if((ss>>d) && ss.eof()) return true;
    return false;
}

bool isFloat(const string& s){
    if(s.empty()) return false;
    stringstream ss(s);
    float f;
    if((ss>>f) && ss.eof()) return true;
    return false;
}

bool Table::validateValue(const string& value,const ColMeta& col){
    switch(col.type){
        case INT:
            if(!isInt(value)){
                cerr<<"Column '"<<col.name<<"' expects an INT.\n";
                return false;
            }
            return true;

        case FLOAT:
            if(!isFloat(value)){
                cerr<<"Column '"<<col.name<<"' expects a FLOAT.\n";
                return false;
            }
            return true;

        case STRING:
            if(!isString(value,col.size)){
                cerr<<"Column '"<<col.name<<"' exceeds maximum length of "
                    <<col.size<<".\n";
                return false;
            }
            return true;

        case BOOL:
            if(!isBool(value)){
                cerr<<"Column '"<<col.name
                    <<"' expects 'true' or 'false'.\n";
                return false;
            }
            return true;
    }
    return false;
}

bool Table::validateRow(const Row& row){
    if(row.values.size()!=meta.columnCount){
        cerr<<"Expected "<<meta.columnCount
            <<" values, but got "<<row.values.size()<<".\n";
        return false;
    }

    for(int i=0;i<meta.columnCount;i++){
        if(!validateValue(row.values[i],meta.columns[i]))
            return false;
    }
    return true;
}

/*
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
        else if(s[i]=='.'){
            if(dot) return false;
            dot=true;
        }
        else return false;
    }
    return digit;
}
*/