#include <iostream>
#include <cassert>
#include <cstdio>
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

void validUpdateTest(Table* table){
    cout<<"[1] Valid updates...\n";
    assert(table->update({{"101","Swayam","IT","9.50","false"}}));
    Record latest=table->latest("101");
    assert(latest.row.values[0]=="101");
    assert(latest.row.values[1]=="Swayam");
    assert(latest.row.values[2]=="IT");
    assert(latest.row.values[3]=="9.500000");
    assert(latest.row.values[4]=="false");
    assert(table->update({{"101","Swayam Jain","IT","9.90","true"}}));
    latest=table->latest("101");
    assert(latest.row.values[1]=="Swayam Jain");
    assert(latest.row.values[3]=="9.900000");
    assert(latest.row.values[4]=="true");
}

void missingRowTest(Table* table){
    cout<<"[2] Missing row update...\n";
    assert(!table->update({{"999","ABC","IT","8.00","true"}}));
}

void datatypeValidationTest(Table* table){
    cout<<"[3] Datatype validation...\n";
    assert(!table->update({{"abc","ABC","IT","8.00","true"}}));
    assert(!table->update({{"101","ABC","IT","xyz","true"}}));
    assert(!table->update({{"101","ABC","IT","8.00","yes"}}));
}

void columnValidationTest(Table* table){
    cout<<"[4] Column validation...\n";
    assert(!table->update({{"101","ABC","IT","8.00"}}));
    assert(!table->update({{"101","ABC","IT","8.00","true","extra"}}));
}

void stringValidationTest(Table* table){
    cout<<"[5] String validation...\n";
    Row longName={{
        "101",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDE",
        "IT",
        "8.00",
        "true"
    }};
    assert(!table->update(longName));
}

void latestVersionTest(Table* table){
    cout<<"[6] Latest version verification...\n";
    Record latest=table->latest("101");
    assert(latest.row.values[1]=="Swayam Jain");
    assert(latest.row.values[3]=="9.900000");
    assert(latest.row.values[4]=="true");
}

void multipleUpdateTest(Table* table){
    cout<<"[7] Multiple updates...\n";
    assert(table->update({{"102","Darshan","CS","8.80","false"}}));
    Record latest=table->latest("102");
    assert(latest.row.values[0]=="102");
    assert(latest.row.values[1]=="Darshan");
    assert(latest.row.values[3]=="8.800000");
    assert(latest.row.values[4]=="false");
    assert(table->update({{"102","Darshan","CS","8.80","false"}}));
    assert(table->update({{"102","Darshan","CS","8.80","false"}}));
    latest=table->latest("102");
    assert(latest.row.values[3]=="8.800000");
    assert(latest.row.values[4]=="false");
}

int main(){
    cout<<"\n========== Update Tests ==========\n\n";
    remove("data/Student.db");
    Catalog catalog;
    Table* table=setupDatabase(catalog);
    validUpdateTest(table);
    missingRowTest(table);
    datatypeValidationTest(table);
    columnValidationTest(table);
    stringValidationTest(table);
    latestVersionTest(table);
    multipleUpdateTest(table);
    cout<<"\n==================================\n";
    cout<<"All update tests passed.\n";
    cout<<"==================================\n";
    return 0;
}