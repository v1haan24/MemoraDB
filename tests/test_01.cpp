#include <iostream>
#include <cassert>
#include <cstdio>
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

    //make table
    assert(catalog.createTable(student)==true);
    assert(catalog.createTable(student)==false);

    //insert
    Table table(student);
    Row r1={{"101","Swayam","IT","7.95","true"}};
    Row r2={{"102","Darshan","CS","7.70","true"}};
    Row r3={{"103","Vihaan","IT","8.26","true"}};
    Row r4={{"104","Reben","EXTC","9.30","true"}};
    assert(table.insert(r1)==true);
    assert(table.insert(r2)==true);
    assert(table.insert(r3)==true);
    assert(table.insert(r4)==true);


    //TESTS : 
    //duplicate pk
    Row duplicate={{"101","Someone","IT","10.00","true"}};
    assert(table.insert(duplicate)==false);

    //invalid int
    Row invalidPK={{"abc","Someone","IT","8.50","true"}};
    assert(table.insert(invalidPK)==false);
    //invalid float
    Row invalidCGPA={{"105","Aryan","CS","abc","true"}};
    assert(table.insert(invalidCGPA)==false);
    //invalid bool
    Row invalidBool={{"106","Harsh","IT","8.90","yes"}};
    assert(table.insert(invalidBool)==false);

    //missing col
    Row missingColumn={{"107","Yash","IT","8.50"}};
    assert(table.insert(missingColumn)==false);
    //extra col
    Row extraColumn={{"108","Aman","IT","8.50","true","extra"}};
    assert(table.insert(extraColumn)==false);

    //long string
    Row longName={{"109",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDE",
        "IT",
        "8.50",
        "true"}};
    assert(table.insert(longName)==false);

    //latest
    Row latest=table.latest("101");
    assert(latest.values[0]=="101");
    assert(latest.values[1]=="Swayam");
    assert(latest.values[2]=="IT");
    assert(latest.values[3]=="7.950000");
    assert(latest.values[4]=="true");
    // ---------- Missing PK ----------
    assert(table.latest("999").values.empty());
    // ---------- Update ----------
    Row update1={{"101","Swayam","IT","9.50","true"}};
    assert(table.update(update1)==true);
    latest=table.latest("101");
    assert(latest.values[0]=="101");
    assert(latest.values[1]=="Swayam");
    assert(latest.values[2]=="IT");
    assert(latest.values[3]=="9.500000");
    assert(latest.values[4]=="true");

    // ---------- Update Missing ----------
    Row updateMissing={{"999","ABC","IT","8.00","true"}};
    assert(table.update(updateMissing)==false);

    // ---------- Delete ----------
    assert(table.deleteRow("101")==true);
    assert(table.deleteRow("101")==false);
    assert(table.deleteRow("999")==false);

    // ---------- Update Deleted ----------
    assert(table.update(update1)==false);

    cout<<"\n====================================\n";
    cout<<"All tests passed successfully!\n";
    cout<<"====================================\n";

    return 0;
}