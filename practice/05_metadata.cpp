#include<iostream>
#include<stdio.h> 
#include<vector>
#include<fstream>
#include<algorithm>
#include<cstring> 
using namespace std;

enum DataType {INT,FLOAT,STRING,BOOL};

struct ColMeta{
    string name;
    DataType type;
    bool isPK;
    int size;
    int offset=0;
    ColMeta()=default;
    ColMeta(string n,DataType t,bool pk,int s=0){
        name=n;type=t;isPK=pk;
        if(type==STRING) size=s;
        else if(type==INT) size=sizeof(int);
        else if(type==FLOAT) size=sizeof(float);
        else if(type==BOOL) size=sizeof(bool);
    }
};
struct TableMeta{
    vector<ColMeta> colum;
    string name;
    int rowSize=0;
};

class Catalog
{
private:
        vector<TableMeta> tables;
public:

    bool createTable(TableMeta& table){
       for(auto &t:tables){
        if(t.name==table.name) return false;
       }
       int pkCount=0;
        for(const auto& col:table.colum) if(col.isPK) pkCount++;
        if(pkCount>1) return false;

       if(table.rowSize==0){
        int size=0;
        for(auto &c:table.colum){
            c.offset=size;
            size+=c.size;
        }
        table.rowSize=size;
       }

       ofstream file(table.name+".db",ios::binary);
       if(!file) return false;
       
    int ntsz=table.name.length();
        file.write(reinterpret_cast<const char*>(&ntsz),sizeof(ntsz));
        file.write(table.name.data(),ntsz);
    file.write(reinterpret_cast<const char*>(&table.rowSize),sizeof(table.rowSize));

    int colcnt=table.colum.size();
    file.write(reinterpret_cast<const char*>(&colcnt),sizeof(colcnt));
    
    for(const auto &c:table.colum){
        int nsz=c.name.length();
        file.write(reinterpret_cast<const char*>(&nsz),sizeof(nsz));
        file.write(c.name.data(),nsz);
        file.write(reinterpret_cast<const char*>(&c.type),sizeof(c.type));
        file.write(reinterpret_cast<const char*>(&c.size),sizeof(c.size));
         file.write(reinterpret_cast<const char*>(&c.offset),sizeof(c.offset));
         file.write(reinterpret_cast<const char*>(&c.isPK),sizeof(c.isPK));
    }
       file.close();
       tables.push_back(table);
        return true;
    }


    TableMeta* getTable(const string& tableName){
        for(auto &t:tables){
            if(t.name==tableName) return &t;
       }
       return NULL;
    }

    TableMeta readMetadata(string fileName){
        ifstream file(fileName,ios::binary);
        if(!file) return {};
        TableMeta temp;

        int len;
        file.read(reinterpret_cast<char*>(&len),sizeof(len));
        temp.name.resize(len);
        file.read(&temp.name[0],len);

        file.read(reinterpret_cast<char*>(&temp.rowSize),sizeof(temp.rowSize));

        int colcnt;
        file.read(reinterpret_cast<char*>(&colcnt),sizeof(colcnt));
        for(int i=0;i<colcnt;i++){
            //length name type size offset ispk
            ColMeta tem;

            int len;
            file.read(reinterpret_cast<char*>(&len),sizeof(len));
            tem.name.resize(len);
            file.read(&tem.name[0],len);

            file.read(reinterpret_cast<char*>(&tem.type),sizeof(tem.type));
            file.read(reinterpret_cast<char*>(&tem.size),sizeof(tem.size));
            file.read(reinterpret_cast<char*>(&tem.offset),sizeof(tem.offset));
            file.read(reinterpret_cast<char*>(&tem.isPK),sizeof(tem.isPK));
            temp.colum.push_back(tem);
        }
        file.close();
        return temp;
    }
    
};

int main(){
    Catalog catalog;

    TableMeta employee;
    employee.name="Employee";
    employee.colum.push_back({"id",INT,true,4});
    employee.colum.push_back({"name",STRING,false,21});
    employee.colum.push_back({"salary",FLOAT,false,4});
    employee.colum.push_back({"active",BOOL,false,1});
    catalog.createTable(employee);

    TableMeta student;
    student.name="Student";
    student.colum.push_back({"rollNo",INT,true,4});
    student.colum.push_back({"studentName",STRING,false,31});
    student.colum.push_back({"cgpa",FLOAT,false,4});
    student.colum.push_back({"hosteller",BOOL,false,1});
    catalog.createTable(student);

    TableMeta course;
    course.name="Course";
    course.colum.push_back({"courseId",INT,true,4});
    course.colum.push_back({"courseName",STRING,false,41});
    course.colum.push_back({"credits",INT,false,4});
    catalog.createTable(course);

    cout<<"All tables created.\n\n";

    TableMeta emp=catalog.readMetadata("Employee.db");
    cout<<"Employee Row Size : "<<emp.rowSize<<endl;

    TableMeta stu=catalog.readMetadata("Student.db");
    cout<<"Student Row Size : "<<stu.rowSize<<endl;

    TableMeta cou = catalog.readMetadata("Course.db");
    cout<<"Course Row Size : "<<cou.rowSize<<endl;

    return 0;
}