#include <iostream>
#include <cassert>
#include "../src/catalog/catalog.h"
#include "../src/storage/table.h"
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

    assert(catalog.createTable(student)==true);
    //assert(catalog.createTable(student)==true);
    assert(catalog.createTable(student)==false);

    Table table(student);
    Row r1={{"101","Swayam","IT","7.95","true"}};
    Row r2={{"102","Darshan","CS","7.70","true"}};
    Row r3={{"103","Vihaan","IT","8.26","true"}};
    Row r4={{"104","Reben","EXTC","9.30","true"}};
    Row duplicate={{"101","Someone","IT","10.00","true"}};
    assert(table.insert(r1)==true);
    //assert(table.insert(r1)==true);
    assert(table.insert(r2)==true);
    assert(table.insert(r3)==true);
    assert(table.insert(r4)==true);
    assert(table.insert(duplicate)==false);
    //assert(table.insert(duplicate)==true);
    Row invalidCGPA={{"105","Aryan","CS","abc","true"}};
    assert(table.insert(invalidCGPA)==false);
    //assert(table.insert(invalidCGPA)==true);
    Row missingColumn={{"106","Yash","IT","8.50"}};
    assert(table.insert(missingColumn)==false);
    //assert(table.insert(missingColumn)==true);
    Row invalidBool={{"107","Harsh","IT","8.90","yes"}};
    assert(table.insert(invalidBool)==false);
    //assert(table.insert(invalidBool)==true);
    cout<<"All tests passed!\n";

    return 0;
}