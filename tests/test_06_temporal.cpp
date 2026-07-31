#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <cstdio>
#include "../src/catalog/catalog.h"
using namespace std;

Table* setupDatabase(Catalog& catalog,uint64_t& t1,uint64_t& t2,uint64_t& t3,uint64_t& t4){
    TableMeta student;
    strncpy(student.name,"Student",tns-1);
    student.name[tns-1]='\0';
    student.columns.push_back({"rollNo",INT,true});
    student.columns.push_back({"name",STRING,false,30});
    student.columns.push_back({"branch",STRING,false,10});
    student.columns.push_back({"cgpa",FLOAT,false});
    student.columns.push_back({"active",BOOL,false});
    assert(catalog.createTable(student));
    Table* table=catalog.getTable("Student");
    assert(table!=nullptr);
    assert(table->insert({{"101","Swayam","IT","8.0","true"}}));
    t1=table->latest("101").timestamp;
    this_thread::sleep_for(chrono::milliseconds(5));
    assert(table->update({{"101","Swayam","IT","8.5","true"}}));
    t2=table->latest("101").timestamp;
    this_thread::sleep_for(chrono::milliseconds(5));
    assert(table->update({{"101","Swayam","CS","9.2","true"}}));
    t3=table->latest("101").timestamp;
    this_thread::sleep_for(chrono::milliseconds(5));
    assert(table->deleteRow("101"));
    t4=table->latest("101").timestamp;
    return table;
}

void historyTest(Table* table){
    cout<<"[1] History...\n";
    auto history=table->showHistory("101");
    assert(history.size()==4);
    assert(history[0].row.values[2]=="IT");
    assert(history[1].row.values[3]=="8.500000");
    assert(history[2].row.values[2]=="CS");
    assert(history[3].deleted);
}

void asOfTest(Table* table,uint64_t t1,uint64_t t2,uint64_t t3){
    cout<<"[2] AS OF query...\n";
    Record record=table->selectAsOf("101",t1);
    assert(record.row.values[3]=="8.000000");
    record=table->selectAsOf("101",t2);
    assert(record.row.values[3]=="8.500000");
    record=table->selectAsOf("101",t3);
    assert(record.row.values[2]=="CS");
}

void betweenTest(Table* table,uint64_t t2,uint64_t t4){
    cout<<"[3] BETWEEN query...\n";
    auto records=table->selectBetween("101",t2,t4);
    assert(records.size()==3);
    assert(records[0].row.values[3]=="8.500000");
    assert(records[1].row.values[2]=="CS");
    assert(records[2].deleted);
}

void snapshotTest(Table* table,uint64_t t1,uint64_t t3,uint64_t t4){
    cout<<"[4] Snapshot...\n";
    auto snapshot=table->snapshot(t1);
    assert(snapshot.size()==1);
    assert(snapshot[0].row.values[3]=="8.000000");
    snapshot=table->snapshot(t3);
    assert(snapshot.size()==1);
    assert(snapshot[0].row.values[2]=="CS");
    snapshot=table->snapshot(t4);
    assert(snapshot.empty());
}

int main(){
    cout<<"\n========= Temporal Tests =========\n\n";
    remove("data/Student.db");
    Catalog catalog;
    uint64_t t1,t2,t3,t4;
    Table* table=setupDatabase(catalog,t1,t2,t3,t4);
    historyTest(table);
    asOfTest(table,t1,t2,t3);
    betweenTest(table,t2,t4);
    snapshotTest(table,t1,t3,t4);
    cout<<"\n==================================\n";
    cout<<"All temporal tests passed.\n";
    cout<<"==================================\n";
    return 0;
}