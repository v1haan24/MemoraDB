#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <cstdio>
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

    assert(catalog.createTable(student));

    Table* table=catalog.getTable("Student");
    assert(table!=nullptr);

    //------------------------------------------
    // INSERT
    //------------------------------------------

    assert(table->insert({{"101","Swayam","IT","8.0","true"}}));

    uint64_t t1 = table->latest("101").timestamp;

    this_thread::sleep_for(chrono::milliseconds(5));

    //------------------------------------------
    // UPDATE 1
    //------------------------------------------

    assert(table->update({{"101","Swayam","IT","8.5","true"}}));

    uint64_t t2=table->latest("101").timestamp;

    this_thread::sleep_for(chrono::milliseconds(5));

    //------------------------------------------
    // UPDATE 2
    //------------------------------------------

    assert(table->update({{"101","Swayam","CS","9.2","true"}}));

    uint64_t t3=table->latest("101").timestamp;

    this_thread::sleep_for(chrono::milliseconds(5));

    //------------------------------------------
    // DELETE
    //------------------------------------------

    assert(table->deleteRow("101"));

    uint64_t t4=table->latest("101").timestamp;

    //------------------------------------------
    // SHOW HISTORY
    //------------------------------------------

    auto hist=table->showHistory("101");

    assert(hist.size()==4);

    assert(hist[0].row.values[2]=="IT");
    assert(hist[1].row.values[3]=="8.500000");
    assert(hist[2].row.values[2]=="CS");
    assert(hist[3].deleted==true);

    //------------------------------------------
    // SELECT AS OF
    //------------------------------------------

    auto r1=table->selectAsOf("101",t1);
    assert(r1.row.values[3]=="8.000000");

    auto r2=table->selectAsOf("101",t2);
    assert(r2.row.values[3]=="8.500000");

    auto r3=table->selectAsOf("101",t3);
    assert(r3.row.values[2]=="CS");

    //------------------------------------------
    // BETWEEN
    //------------------------------------------

    auto between=table->selectBetween("101",t2,t4);

    assert(between.size()==3);

    assert(between[0].row.values[3]=="8.500000");
    assert(between[1].row.values[2]=="CS");
    assert(between[2].deleted);

    //------------------------------------------
    // SNAPSHOT
    //------------------------------------------

    auto snap1=table->snapshot(t1);

    assert(snap1.size()==1);
    assert(snap1[0].row.values[3]=="8.000000");

    auto snap2=table->snapshot(t3);

    assert(snap2.size()==1);
    assert(snap2[0].row.values[2]=="CS");

    auto snap3=table->snapshot(t4);

    assert(snap3.empty());

    //------------------------------------------

    cout<<"\n====================================\n";
    cout<<"Temporal tests passed successfully!\n";
    cout<<"====================================\n";

    return 0;
}