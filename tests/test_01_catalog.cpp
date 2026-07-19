#include <iostream>
#include <cassert>
#include <cstdio>
#include <fstream>
#include "../src/catalog/catalog.h"
using namespace std;

int main(){
    remove("data/Student.db");
    Catalog catalog;
    TableMeta student;
    strncpy(student.name,"Student",tns-1);
    student.name[tns-1]='\0';
    student.columns.push_back({"rollNo",INT,true});
    student.columns.push_back({"name",STRING,false,30});
    student.columns.push_back({"branch",STRING,false,10});
    student.columns.push_back({"cgpa",FLOAT,false});
    student.columns.push_back({"active",BOOL,false});
    assert(catalog.createTable(student)==true); //create
    assert(catalog.createTable(student)==false); //dup
    ifstream check("data/Student.db",ios::binary); //file create
    assert(check.good());
    check.close();
    Table* table=catalog.getTable("Student"); //gettable
    assert(table!=nullptr);
    assert(catalog.getTable("Dummy")==nullptr);
    assert(catalog.getTable("")==nullptr);

    const TableMeta& meta=table->getMeta(); //metdadata
    assert(strcmp(meta.name,"Student")==0);
    assert(meta.columnCount==5);
    assert(meta.columns[0].type==INT);
    assert(meta.columns[0].isPK==true);
    assert(meta.columns[1].type==STRING);
    assert(meta.columns[1].size==30);
    assert(meta.columns[2].type==STRING);
    assert(meta.columns[2].size==10);
    assert(meta.columns[3].type==FLOAT);
    assert(meta.columns[4].type==BOOL);
    assert(meta.columns[0].offset==0); //offsets
    assert(meta.columns[1].offset==4);
    assert(meta.columns[2].offset==34);
    assert(meta.columns[3].offset==44);
    assert(meta.columns[4].offset==48);
    assert(meta.payloadSize==49);

    Catalog restarted; //restart
    Table* table2=restarted.getTable("Student");
    assert(table2!=nullptr);
    assert(restarted.getTable("Dummy")==nullptr);
    assert(restarted.getTable("")==nullptr);
    const TableMeta& meta2=table2->getMeta();
    assert(strcmp(meta2.name,"Student")==0);
    assert(meta2.columnCount==5);
    assert(meta2.payloadSize==49);
    assert(restarted.createTable(student)==false); //dup

    cout<<"Catalog tests passed successfully!\n";
    return 0;
}