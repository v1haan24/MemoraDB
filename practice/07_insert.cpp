#include<iostream>
#include<stdio.h> 
#include<vector>
#include<string> 
#include<fstream>
#include<algorithm>
#include<cstring> 
#include <unordered_set>
#include <unordered_map>
#include<chrono>
#include<climits>
#include<cctype>
using namespace std;

#define cns 30 //fixed col name size
#define tns 30 //fixed table name size

enum DataType {INT,FLOAT,STRING,BOOL};

struct ColMeta{
    char name[cns];
    bool isPK;
    DataType type;
    int size;
    int offset=0;
    ColMeta()=default;
    ColMeta(string n,DataType t,bool pk,int s=0){
        strncpy(name,n.c_str(),cns-1);
        name[cns-1]='\0';
        type=t;isPK=pk;
        if(type==STRING) size=s;
        else if(type==INT) size=sizeof(int);
        else if(type==FLOAT) size=sizeof(float);
        else if(type==BOOL) size=sizeof(bool);
    }
};

struct TableMeta{
    int metadataSize;
    vector<ColMeta> columns;
    int rowCount=0;
    int columnCount;
    char name[tns];
    int payloadSize=0;
};

struct Row{
    vector<string> values; //assuming parser gives string as output
};

struct RecordVersion{
    uint64_t timestamp;
    uint64_t offset;
};

template<typename T>
void writeBinary(ostream& file,const T& value){
    file.write(reinterpret_cast<const char*>(&value),sizeof(T));
}
template<typename T>
void readBinary(istream& file,T& value){
    file.read(reinterpret_cast<char*>(&value),sizeof(T));
}

void writeColumn(ofstream& file,const ColMeta& col){
    file.write(col.name,cns);
    writeBinary(file,col.type);
    writeBinary(file,col.size);
    writeBinary(file,col.offset);
    writeBinary(file,col.isPK);
}

void readColumn(ifstream& file,ColMeta& col){
    file.read(col.name,cns);
    readBinary(file,col.type);
    readBinary(file,col.size);
    readBinary(file,col.offset);
    readBinary(file,col.isPK);
}

class Catalog{
    vector<TableMeta> tables;
    bool exist(const string& tableName){
        for(auto &t:tables) if(strcmp(t.name,tableName.c_str())==0) return true;
        return false;
    }

    void CalcOffset(TableMeta& table){
        int size=0;
        for(auto &c:table.columns){
            c.offset=size;
            size+=c.size;
        }
        table.payloadSize=size;
    }
public:
    bool createTable(TableMeta& table){
        if(strlen(table.name)>=tns) return false;
        if(table.columns.empty()) return false;
        unordered_set<string> names;
        int pkCount=0;
        for(const auto& col:table.columns){
            if(strlen(col.name)>=cns) return false;
            if(!names.insert(col.name).second) return false;
            if(col.type==STRING && col.size<=0) return false;
            if(col.isPK) pkCount++;
        }
        if(pkCount!=1) return false;
        if(exist(table.name)) return false;


        CalcOffset(table);
        if(table.payloadSize==0) return false;

        ofstream file(string(table.name)+".db",ios::binary);
        if(!file) return false;

        table.columnCount=table.columns.size();

        table.metadataSize=46+43*table.columnCount; //<--- if change in cns,tns change this too

        writeBinary(file,table.metadataSize);

        file.write(table.name,tns);
        writeBinary(file,table.rowCount);
        writeBinary(file,table.payloadSize);

        writeBinary(file,table.columnCount);
        for(const auto& col:table.columns) writeColumn(file,col);
        
        tables.push_back(table);
        return true;
    }

    TableMeta* getTable(const string& tableName){
        for(auto &t:tables){
            if(strcmp(t.name,tableName.c_str())==0) return &t;
       }
       return NULL;
    }

    TableMeta readMetadata(string fileName){
        ifstream file(fileName,ios::binary);
        if(!file) return {};
        TableMeta temp;
        readBinary(file,temp.metadataSize);
        file.read(temp.name,tns);
        readBinary(file,temp.rowCount);
        readBinary(file,temp.payloadSize);

        readBinary(file,temp.columnCount);
        for(int i=0;i<temp.columnCount;i++){
            ColMeta col;
            readColumn(file,col);
            temp.columns.push_back(col);
        }
        return temp;
    }
};
//------------------------------------------------------------

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
        else if(s[i]=='.'){
            if(dot) return false;
            dot=true;
        }
        else return false;
    }
    return digit;
}

class Table{
    TableMeta meta;
    unordered_map<string,vector<RecordVersion>> history;
public:
    Table(const TableMeta& metadata){
        meta=metadata;
    }
    bool insert(const Row& row){
        if(row.values.size()!=meta.columnCount) return false;

        for(int i=0;i<meta.columnCount;i++){
            if(meta.columns[i].type==INT){
                if(!isInt(row.values[i])) return false;
            }
            if(meta.columns[i].type==FLOAT){
                if(!isFloat(row.values[i])) return false;
            }
            if(meta.columns[i].type==STRING){
                if(!isString(row.values[i],meta.columns[i].size)) return false;
            }
            if(meta.columns[i].type==BOOL){
                if(!isBool(row.values[i])) return false;
            }
        }

        string pk;
        for(int i=0;i<meta.columnCount;i++){
            if(meta.columns[i].isPK){
                pk=row.values[i];
                break;
            }
        }
        if(history.count(pk)) return false;

        fstream file(string(meta.name)+".db",ios::binary|ios::in|ios::out);
        if(!file) return false;

        file.seekp(0,ios::end);
        uint64_t offset=file.tellp();
        uint64_t timestamp=
        chrono::duration_cast<chrono::milliseconds>(
            chrono::system_clock::now().time_since_epoch()
        ).count();
        bool deleted=false;
        writeBinary(file,timestamp);
        writeBinary(file,deleted);

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
                memcpy(
                    temp.data(),
                    row.values[i].data(),
                    min((int)row.values[i].size(),meta.columns[i].size-1)
                );
                file.write(temp.data(),meta.columns[i].size);
            }
        }

        history[pk].push_back({timestamp,offset});
        meta.rowCount++;
        file.seekp(sizeof(int)+tns,ios::beg);
        writeBinary(file,meta.rowCount);
        file.close();

        return true;
    }
};

//-----------------------------------------------------------------------------------------
int main(){

    Catalog catalog;

    TableMeta employee;
    strncpy(employee.name,"Employee",tns);
    employee.columns.push_back({"id",INT,true});
    employee.columns.push_back({"name",STRING,false,21});
    employee.columns.push_back({"salary",FLOAT,false});
    employee.columns.push_back({"active",BOOL,false});
    cout<<"Create Table : "<<catalog.createTable(employee)<<endl;

    TableMeta meta=catalog.readMetadata("Employee.db");
    Table table(meta);
    Row r1;
    r1.values={"1","Alice","50000.5","true"};
    Row r2;
    r2.values={"2","Bob","75000","false"};
    Row r3;
    r3.values={"1","Charlie","90000","true"};   //test dup pk

    cout<<"Insert 1 : "<<table.insert(r1)<<endl;
    cout<<"Insert 2 : "<<table.insert(r2)<<endl;
    cout<<"Insert 3 : "<<table.insert(r3)<<endl;

    return 0;
}