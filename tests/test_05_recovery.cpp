#include <iostream>
#include <cassert>
#include <cstdio>
#include "../src/catalog/catalog.h"
using namespace std;

void createDatabase(){
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
    assert(table->insert({{"101","Swayam","IT","7.95","true"}}));
    assert(table->insert({{"102","Darshan","CS","7.70","true"}}));
    assert(table->insert({{"103","Vihaan","IT","8.26","true"}}));
    assert(table->update({{"101","Swayam","IT","9.50","false"}}));
    assert(table->update({{"102","Darshan","CS","8.80","true"}}));
    assert(table->deleteRow("103"));
}

Table* restartDatabase(Catalog& catalog){
    Table* table=catalog.getTable("Student");
    assert(table!=nullptr);
    return table;
}

void recoveredDataTest(Table* table){
    cout<<"[1] Recover latest records...\n";
    Record latest=table->latest("101");
    assert(latest.row.values[0]=="101");
    assert(latest.row.values[1]=="Swayam");
    assert(latest.row.values[2]=="IT");
    assert(latest.row.values[3]=="9.500000");
    assert(latest.row.values[4]=="false");
    latest=table->latest("102");
    assert(latest.row.values[0]=="102");
    assert(latest.row.values[1]=="Darshan");
    assert(latest.row.values[2]=="CS");
    assert(latest.row.values[3]=="8.800000");
    assert(latest.row.values[4]=="true");
}

void deletedRowRecoveryTest(Table* table){
    cout<<"[2] Deleted row recovery...\n";
    assert(!table->update({{"103","Vihaan","IT","9.99","true"}}));
}

void duplicatePrimaryKeyTest(Table* table){
    cout<<"[3] Duplicate primary key...\n";
    assert(!table->insert({{"101","ABC","IT","1.00","true"}}));
    assert(!table->insert({{"102","ABC","IT","1.00","true"}}));
}

void continueOperationsTest(Table* table){
    cout<<"[4] Continue after recovery...\n";
    assert(table->insert({{"104","Reben","EXTC","9.20","true"}}));
    assert(table->insert({{"105","Aryan","CS","8.55","true"}}));
    Record latest=table->latest("104");
    assert(latest.row.values[0]=="104");
    assert(latest.row.values[1]=="Reben");
    latest=table->latest("105");
    assert(latest.row.values[0]=="105");
    assert(latest.row.values[1]=="Aryan");
}

int main(){
    cout<<"\n========= Recovery Tests =========\n\n";
    remove("data/Student.db");
    createDatabase();
    Catalog catalog;
    Table* table=restartDatabase(catalog);
    recoveredDataTest(table);
    deletedRowRecoveryTest(table);
    duplicatePrimaryKeyTest(table);
    continueOperationsTest(table);
    cout<<"\n==================================\n";
    cout<<"All recovery tests passed.\n";
    cout<<"==================================\n";
    return 0;
}