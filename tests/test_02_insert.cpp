#include <iostream>
#include <cassert>
#include "../src/catalog/catalog.h"
using namespace std;

Table* setupDatabase(Catalog& catalog){
    Table* table=catalog.getTable("Student");
    assert(table!=nullptr);
    return table;
}

void validInsertTest(Table* table){
    cout<<"[1] Valid inserts...\n";
    Row r1={{"101","Swayam","IT","7.95","true"}};
    Row r2={{"102","Darshan","CS","7.70","true"}};
    Row r3={{"103","Vihaan","IT","8.26","true"}};
    Row r4={{"104","Reben","EXTC","9.30","true"}};
    assert(table->insert(r1));
    assert(table->insert(r2));
    assert(table->insert(r3));
    assert(table->insert(r4));
    Record latest=table->latest("101");
    assert(latest.row.values[1]=="Swayam");
    latest=table->latest("102");
    assert(latest.row.values[1]=="Darshan");
    latest=table->latest("103");
    assert(latest.row.values[1]=="Vihaan");
    latest=table->latest("104");
    assert(latest.row.values[1]=="Reben");
}

void duplicatePrimaryKeyTest(Table* table){
    cout<<"[2] Duplicate primary key...\n";
    Row duplicate={{"101","Someone","IT","10.00","true"}};
    assert(!table->insert(duplicate));
    Record latest=table->latest("101");
    assert(latest.row.values[1]=="Swayam");
}

void datatypeValidationTest(Table* table){
    cout<<"[3] Datatype validation...\n";
    assert(!table->insert({{"abc","Someone","IT","8.50","true"}}));
    assert(!table->insert({{"105","Aryan","CS","abc","true"}}));
    assert(!table->insert({{"106","Harsh","IT","8.90","yes"}}));
}

void columnValidationTest(Table* table){
    cout<<"[4] Column validation...\n";
    assert(!table->insert({{"107","Yash","IT","8.50"}}));
    assert(!table->insert({{"108","Aman","IT","8.50","true","extra"}}));
}

void stringValidationTest(Table* table){
    cout<<"[5] String validation...\n";
    Row longName={{
        "109",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDE",
        "IT",
        "8.50",
        "true"
    }};
    assert(!table->insert(longName));
    Row boundary={{
        "111",
        "ABCDEFGHIJKLMNOPQRSTUVWX12345",
        "IT",
        "8.50",
        "true"
    }};
    assert(boundary.values[1].size()==29);
    assert(table->insert(boundary));
}

void emptyFieldTest(Table* table){
    cout<<"[6] Empty field validation...\n";
    assert(!table->insert({{"","ABC","IT","8.50","true"}}));
    Row emptyName={{"110","","IT","8.50","true"}};
    assert(table->insert(emptyName));
    Record latest=table->latest("110");
    assert(latest.row.values[1]=="");
}

void duplicateInsertTest(Table* table){
    cout<<"[7] Repeated duplicate inserts...\n";
    assert(!table->insert({{"101","Swayam","IT","7.95","true"}}));
    assert(!table->insert({{"102","Darshan","CS","7.70","true"}}));
    assert(!table->insert({{"103","Vihaan","IT","8.26","true"}}));
    assert(!table->insert({{"104","Reben","EXTC","9.30","true"}}));
}

int main(){
    cout<<"\n========== Insert Tests ==========\n\n";
    Catalog catalog;
    Table* table=setupDatabase(catalog);
    validInsertTest(table);
    duplicatePrimaryKeyTest(table);
    datatypeValidationTest(table);
    columnValidationTest(table);
    stringValidationTest(table);
    emptyFieldTest(table);
    duplicateInsertTest(table);
    cout<<"\n==================================\n";
    cout<<"All insert tests passed.\n";
    cout<<"==================================\n";
    return 0;
}