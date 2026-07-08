#include<iostream>
#include<stdio.h> 
#include<vector>
#include<fstream>
#include<algorithm>
#include<cstring> 
#include <unordered_set>
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
    int rowSize=0;
};

template<typename T>
void writeBinary(ofstream& file,const T& value){
    file.write(reinterpret_cast<const char*>(&value),sizeof(T));
}
template<typename T>
void readBinary(ifstream& file,T& value){
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
        table.rowSize=size;
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
        if(table.rowSize==0) return false;

        ofstream file(string(table.name)+".db",ios::binary);
        if(!file) return false;

        table.columnCount=table.columns.size();

        table.metadataSize=46+43*table.columnCount; //<--- if change in cns,tns change this too

        writeBinary(file,table.metadataSize);

        file.write(table.name,tns);
        writeBinary(file,table.rowCount);
        writeBinary(file,table.rowSize);

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
        readBinary(file,temp.rowSize);

        readBinary(file,temp.columnCount);
        for(int i=0;i<temp.columnCount;i++){
            ColMeta col;
            readColumn(file,col);
            temp.columns.push_back(col);
        }
        return temp;
    }
};

int main(){
    Catalog catalog;

    TableMeta employee;
    strcpy(employee.name,"Employee");
    employee.columns.push_back({"id", INT, true});
    employee.columns.push_back({"name", STRING, false, 21});
    employee.columns.push_back({"salary", FLOAT, false});
    employee.columns.push_back({"active", BOOL, false});
    catalog.createTable(employee);

    TableMeta student;
    strcpy(student.name,"Student");
    student.columns.push_back({"rollNo", INT, true});
    student.columns.push_back({"studentName", STRING, false, 31});
    student.columns.push_back({"cgpa", FLOAT, false});
    student.columns.push_back({"hosteller", BOOL, false});
    catalog.createTable(student);

    TableMeta course;
    strcpy(course.name,"Course");
    course.columns.push_back({"courseId", INT, true});
    course.columns.push_back({"courseName", STRING, false, 41});
    course.columns.push_back({"credits", INT, false});
    catalog.createTable(course);

    cout << "All tables created.\n\n";

    TableMeta emp = catalog.readMetadata("Employee.db");
    cout << emp.name << " Row Size : " << emp.rowSize << endl;

    TableMeta stu = catalog.readMetadata("Student.db");
    cout << stu.name << " Row Size : " << stu.rowSize << endl;

    TableMeta cou = catalog.readMetadata("Course.db");
    cout << cou.name << " Row Size : " << cou.rowSize << endl;

    return 0;
}