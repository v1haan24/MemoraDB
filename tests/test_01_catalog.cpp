#include <iostream>
#include <cassert>
#include <cstdio>
#include <fstream>
#include "../src/catalog/catalog.h"
using namespace std;

void setupStudentMeta(TableMeta& student){
    strncpy(student.name,"Student",tns-1);
    student.name[tns-1]='\0';
    student.columns.push_back({"rollNo",INT,true});
    student.columns.push_back({"name",STRING,false,30});
    student.columns.push_back({"branch",STRING,false,10});
    student.columns.push_back({"cgpa",FLOAT,false});
    student.columns.push_back({"active",BOOL,false});
}

void createTableTest(Catalog& catalog,TableMeta& student){
    cout<<"[1] Creating table...\n";
    assert(catalog.createTable(student));
    assert(catalog.getTable("Student")!=nullptr);
    ifstream db("data/Student.db",ios::binary);
    assert(db.good());
}

void duplicateTableTest(Catalog& catalog,TableMeta& student){
    cout<<"[2] Duplicate table detection...\n";
    assert(!catalog.createTable(student));
}

void getTableTest(Catalog& catalog){
    cout<<"[3] Table lookup...\n";
    assert(catalog.getTable("Student")!=nullptr);
    assert(catalog.getTable("Dummy")==nullptr);
    assert(catalog.getTable("")==nullptr);
}

void metadataTest(Catalog& catalog){
    cout<<"[4] Metadata verification...\n";
    Table* table=catalog.getTable("Student");
    assert(table!=nullptr);
    const TableMeta& meta=table->getMeta();
    assert(strcmp(meta.name,"Student")==0);
    assert(meta.columnCount==5);
    assert(meta.payloadSize==49);
    const auto& cols=meta.columns;
    assert(cols[0].type==INT);
    assert(cols[0].isPK);
    assert(cols[1].type==STRING);
    assert(cols[1].size==30);
    assert(cols[2].type==STRING);
    assert(cols[2].size==10);
    assert(cols[3].type==FLOAT);
    assert(cols[4].type==BOOL);
    assert(cols[0].offset==0);
    assert(cols[1].offset==4);
    assert(cols[2].offset==34);
    assert(cols[3].offset==44);
    assert(cols[4].offset==48);
}

void recoveryTest(){
    cout<<"[5] Catalog recovery...\n";
    Catalog restarted;
    Table* table=restarted.getTable("Student");
    assert(table!=nullptr);
    const TableMeta& meta=table->getMeta();
    assert(strcmp(meta.name,"Student")==0);
    assert(meta.columnCount==5);
    assert(meta.payloadSize==49);
    assert(restarted.getTable("Dummy")==nullptr);
    assert(restarted.getTable("")==nullptr);
}

int main(){
    remove("data/Student.db");

    cout<<"\n========== Catalog Tests ==========\n\n";

    Catalog catalog;

    TableMeta student;
    setupStudentMeta(student);

    createTableTest(catalog,student);
    duplicateTableTest(catalog,student);
    getTableTest(catalog);
    metadataTest(catalog);
    recoveryTest();

    cout<<"\n===================================\n";
    cout<<"All catalog tests passed.\n";
    cout<<"===================================\n";

    return 0;
}