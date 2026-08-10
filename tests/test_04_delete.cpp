#include <iostream>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include "../src/catalog/catalog.h"
using namespace std;

Table* setupDatabase(Catalog& catalog){
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
    return table;
}

void validDeleteTest(Table* table){
    cout<<"[1] Valid delete...\n";
    assert(table->deleteRow("101"));
    assert(!table->deleteRow("101"));
}

void missingRowTest(Table* table){
    cout<<"[2] Missing row delete...\n";
    assert(!table->deleteRow("999"));
    assert(!table->deleteRow("500"));
}

void deletedRowRestrictionTest(Table* table){
    cout<<"[3] Deleted row restrictions...\n";
    assert(!table->update({{"101","Swayam","IT","9.50","true"}}));
    assert(!table->insert({{"101","Swayam","IT","7.95","true"}}));
}

void remainingRowsTest(Table* table){
    cout<<"[4] Remaining rows...\n";
    Record latest=table->latest("102");
    assert(latest.row.values[0]=="102");
    assert(latest.row.values[1]=="Darshan");
    assert(latest.row.values[2]=="CS");
    assert(latest.row.values[3]=="7.700000");
    assert(latest.row.values[4]=="true");
    latest=table->latest("103");
    assert(latest.row.values[0]=="103");
    assert(latest.row.values[1]=="Vihaan");
    assert(latest.row.values[2]=="IT");
    assert(latest.row.values[3]=="8.260000");
    assert(latest.row.values[4]=="true");
}

void deleteRemainingRowsTest(Table* table){
    cout<<"[5] Delete remaining rows...\n";
    assert(table->deleteRow("102"));
    assert(table->deleteRow("103"));
    assert(!table->deleteRow("102"));
    assert(!table->deleteRow("103"));
}

void updateDeletedRowsTest(Table* table){
    cout<<"[6] Update deleted rows...\n";
    assert(!table->update({{"102","Darshan","CS","8.50","false"}}));
    assert(!table->update({{"103","Vihaan","IT","9.00","true"}}));
}

int main(){
    cout<<"\n========== Delete Tests ==========\n\n";
    filesystem::remove_all("data/Student");
    Catalog catalog;
    Table* table=setupDatabase(catalog);
    validDeleteTest(table);
    missingRowTest(table);
    deletedRowRestrictionTest(table);
    remainingRowsTest(table);
    deleteRemainingRowsTest(table);
    updateDeletedRowsTest(table);
    cout<<"\n==================================\n";
    cout<<"All delete tests passed.\n";
    cout<<"==================================\n";
    return 0;
}
