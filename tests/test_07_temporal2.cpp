#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cstdio>
#include "../src/catalog/catalog.h"
using namespace std;

Table* setupDatabase(Catalog& catalog,uint64_t& t1,uint64_t& t2,uint64_t& t3){
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
    assert(table->insert({{"101","Swayam","IT","8.00","true"}}));
    assert(table->insert({{"102","Darshan","CS","7.80","true"}}));
    assert(table->insert({{"103","Vihaan","IT","9.10","true"}}));
    t1=max({
        table->latest("101").timestamp,
        table->latest("102").timestamp,
        table->latest("103").timestamp
    });
    this_thread::sleep_for(chrono::milliseconds(5));
    assert(table->update({{"101","Swayam","IT","8.50","true"}}));
    assert(table->update({{"102","Darshan","AIDS","8.40","true"}}));
    t2=max(
        table->latest("101").timestamp,
        table->latest("102").timestamp
    );
    this_thread::sleep_for(chrono::milliseconds(5));
    assert(table->deleteRow("103"));
    assert(table->insert({{"104","Reben","EXTC","9.40","true"}}));
    t3=max(
        table->latest("103").timestamp,
        table->latest("104").timestamp
    );
    return table;
}

bool contains(const vector<Record>& rows,const string& pk){
    for(const auto& r:rows){
        if(r.row.values[0]==pk)
            return true;
    }
    return false;
}

void snapshot1Test(Table* table,uint64_t t1){
    cout<<"[1] Snapshot before updates...\n";
    auto rows=table->snapshot(t1);
    assert(rows.size()==3);
    assert(contains(rows,"101"));
    assert(contains(rows,"102"));
    assert(contains(rows,"103"));
}

void snapshot2Test(Table* table,uint64_t t2){
    cout<<"[2] Snapshot after updates...\n";
    auto rows=table->snapshot(t2);
    assert(rows.size()==3);
    assert(contains(rows,"101"));
    assert(contains(rows,"102"));
    assert(contains(rows,"103"));
    assert(table->selectAsOf("101",t2).row.values[3]=="8.500000");
    assert(table->selectAsOf("102",t2).row.values[2]=="AIDS");
    assert(table->selectAsOf("103",t2).row.values[0]=="103");
}

void snapshot3Test(Table* table,uint64_t t3){
    cout<<"[3] Snapshot after delete...\n";
    auto rows=table->snapshot(t3);
    assert(rows.size()==3);
    assert(contains(rows,"101"));
    assert(contains(rows,"102"));
    assert(contains(rows,"104"));
    assert(table->selectAsOf("101",t3).row.values[0]=="101");
    assert(table->selectAsOf("102",t3).row.values[0]=="102");
    assert(table->selectAsOf("104",t3).row.values[0]=="104");
}

void historyTest(Table* table){
    cout<<"[4] History verification...\n";
    auto history101=table->showHistory("101");
    auto history102=table->showHistory("102");
    auto history103=table->showHistory("103");
    assert(history101.size()==2);
    assert(history102.size()==2);
    assert(history103.size()==2);
    assert(history103.back().deleted);
}

void latestStateTest(Table* table){
    cout<<"[5] Latest state...\n";
    Record latest=table->latest("101");
    assert(latest.row.values[3]=="8.500000");
    latest=table->latest("102");
    assert(latest.row.values[2]=="AIDS");
    latest=table->latest("104");
    assert(latest.row.values[1]=="Reben");
}

int main(){
    cout<<"\n======== Temporal Snapshot Tests ========\n\n";
    remove("data/Student.db");
    Catalog catalog;
    uint64_t t1,t2,t3;
    Table* table=setupDatabase(catalog,t1,t2,t3);
    snapshot1Test(table,t1);
    snapshot2Test(table,t2);
    snapshot3Test(table,t3);
    historyTest(table);
    latestStateTest(table);
    cout<<"\n=========================================\n";
    cout<<"All temporal snapshot tests passed.\n";
    cout<<"=========================================\n";
    return 0;
}