#include "table.h"
#include "serializer.h"
#include <sstream>
#include <iostream>
#include <cctype>

string Table::getPrimaryKey(const Row& row){
    string pk;
    for(int i=0;i<meta.columnCount;i++){
        if(meta.columns[i].isPK){
            pk=row.values[i];
            break;
        }
    }
    return pk;
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

bool isBool(const string& s){
    return s=="true" || s=="false";
}

bool isString(const string& s,int maxSize){
    return s.length()<=maxSize;
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


void Table::printDatabase(){
    fstream file("data/"+string(meta.name)+".db",ios::binary|ios::in);
    if(!file){ cerr<<"Failed to open table file '"<<meta.name<<"'.\n"; return;}
    cout<<"\nTable Name : "<<meta.name<<'\n';
    cout<<"Metadata Size : "<<meta.metadataSize<<'\n';
    cout<<"Payload Size : "<<meta.payloadSize<<'\n';
    cout<<"Column Count : "<<meta.columnCount<<'\n';
    cout<<"Recovered Rows : "<<meta.rowCount<<'\n';
    cout<<"Record Size : "<<rhsz+meta.payloadSize<<"\n\n";
    cout<<"Columns\n";
    for(int i=0;i<meta.columnCount;i++){
        cout<<"\nColumn "<<i+1<<'\n';
        cout<<"Name : "<<meta.columns[i].name<<'\n';
        cout<<"Type : ";
        switch(meta.columns[i].type){
            case INT: cout<<"INT"; break;
            case FLOAT: cout<<"FLOAT"; break;
            case STRING: cout<<"STRING"; break;
            case BOOL: cout<<"BOOL"; break;
        }
        cout<<'\n';
        cout<<"Size : "<<meta.columns[i].size<<'\n';
        cout<<"Offset : "<<meta.columns[i].offset<<'\n';
        cout<<"Primary Key : "<<(meta.columns[i].isPK?"Yes":"No")<<'\n';
    }
    cout<<"\nRecords\n";
    file.seekg(meta.metadataSize,ios::beg);
    int cnt=1;
    int deletedCnt=0;
    while(true){
        uint64_t offset=file.tellg();
        uint64_t timestamp;
        bool deleted;
        readBinary(file,timestamp);
        if(!file) break;
        readBinary(file,deleted);
        Row row=readPayload(file);
        if(deleted) deletedCnt++;
        cout<<"\nRecord "<<cnt<<'\n';
        cout<<"Offset : "<<offset<<'\n';
        cout<<"Timestamp : "<<timestamp<<'\n';
        cout<<"Deleted : "<<(deleted?"true":"false")<<'\n';
        for(int i=0;i<meta.columnCount;i++)
            cout<<meta.columns[i].name<<" : "<<row.values[i]<<'\n';
        cout<<"Next Offset : "<<file.tellg()<<'\n';
        cnt++;
    }
    cout<<"\nSummary\n";
    cout<<"Total Records : "<<cnt-1<<'\n';
    cout<<"Deleted Records : "<<deletedCnt<<'\n';
    cout<<"Live Rows : "<<history.size()<<'\n';
    cout<<"\nEnd of File\n";
    file.close();
}