#include "table.h"
#include "serializer.h"
#include <chrono>
#include <cstring>
#include<climits>
#include <algorithm>
#include <fstream>

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

bool Table::validateRow(const Row& row){
    if(row.values.size()!=meta.columnCount) return false;
    for(int i=0;i<meta.columnCount;i++){
        if(meta.columns[i].type==INT) if(!isInt(row.values[i])) return false;
        if(meta.columns[i].type==FLOAT) if(!isFloat(row.values[i])) return false;
        if(meta.columns[i].type==STRING) if(!isString(row.values[i],meta.columns[i].size)) return false;
        if(meta.columns[i].type==BOOL) if(!isBool(row.values[i])) return false;
    }
    return true;
}

uint64_t Table::writeHeader(fstream& file){
    uint64_t timestamp =
        chrono::duration_cast<chrono::milliseconds>(
            chrono::system_clock::now().time_since_epoch()
        ).count();
    bool deleted=false;
    writeBinary(file,timestamp);
    writeBinary(file,deleted);
    return timestamp;
}

void Table::writePayload(fstream& file,const Row& row){
    for(int i=0;i<meta.columnCount;i++){
        if(meta.columns[i].type==INT){
            int x=stoi(row.values[i]);
            writeBinary(file,x);
        }
        else if(meta.columns[i].type==FLOAT){
            float x=stof(row.values[i]);
            writeBinary(file,x);
        }
        else if(meta.columns[i].type==BOOL){
            bool x=(row.values[i]=="true");
            writeBinary(file,x);
        }
        else if(meta.columns[i].type==STRING){
            vector<char> temp(meta.columns[i].size,'\0');
            memcpy(temp.data(),
            row.values[i].data(),
            min((int)row.values[i].size(),meta.columns[i].size-1));
            file.write(temp.data(),meta.columns[i].size);
        }
    }
}

string Table::getPrimaryKey(const Row& row){
    string pk;
    for(int i=0;i<meta.columnCount;i++){
        if(meta.columns[i].isPK){ pk=row.values[i]; break;}
    }
    return pk;
}

bool Table::insert(const Row& row){
    if(!validateRow(row)) return false;
    string pk = getPrimaryKey(row);
    if(history.count(pk)) return false;
    fstream file("data/"+string(meta.name)+".db",ios::binary|ios::in|ios::out);
    if(!file) return false;
    file.seekp(0,ios::end); 
    uint64_t offset=file.tellp();
    uint64_t t=writeHeader(file);
    writePayload(file,row);
    history[pk].push_back({t,offset});
    meta.rowCount++;
    file.seekp(sizeof(int)+tns,ios::beg);
    writeBinary(file,meta.rowCount);
    file.close();
    return true;
}