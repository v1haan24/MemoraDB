#include "table.h"
#include <sstream>

bool isBool(const std::string& s){
    return s=="true" || s=="false";
}

bool isString(const std::string& s,int maxSize){
    return s.length()<=maxSize-1;
}

bool isInt(const std::string& s){
    if(s.empty()) return false;
    std::stringstream ss(s);
    int d;
    if((ss>>d) && ss.eof()) return true;
    return false;
}

bool isFloat(const std::string& s){
    if(s.empty()) return false;
    std::stringstream ss(s);
    float f;
    if((ss>>f) && ss.eof()) return true;
    return false;
}

bool validateValue(const std::string& value,const ColMeta& col){
    switch(col.type){
        case INT:
            if(!isInt(value)){
                std::cerr<<"Column '"<<col.name<<"' expects an INT.\n";
                return false;
            }
            return true;

        case FLOAT:
            if(!isFloat(value)){
                std::cerr<<"Column '"<<col.name<<"' expects a FLOAT.\n";
                return false;
            }
            return true;

        case STRING:
            if(!isString(value,col.size)){
                std::cerr<<"Column '"<<col.name<<"' exceeds maximum length of "
                    <<col.size<<".\n";
                return false;
            }
            return true;

        case BOOL:
            if(!isBool(value)){
                std::cerr<<"Column '"<<col.name<<"' expects 'true' or 'false'.\n";
                return false;
            }
            return true;
    }
    return false;
}

bool Table::validateRow(const Row& row){
    if(row.values.size()!=meta.columnCount){
        std::cerr<<"Expected "<<meta.columnCount<<" values, but got "<<row.values.size()<<".\n";
        return false;
    }

    for(int i=0;i<meta.columnCount;i++){
        if(!validateValue(row.values[i],meta.columns[i]))
            return false;
    }
    return true;
}