#include <iostream>
#include <cassert>
#include "../src/catalog/catalog.h"
using namespace std;

int main(){
    Catalog catalog;
    Table* table=catalog.getTable("Student");
    assert(table!=nullptr);

    // ---------- Valid Inserts ----------
    Row r1={{"101","Swayam","IT","7.95","true"}};
    Row r2={{"102","Darshan","CS","7.70","true"}};
    Row r3={{"103","Vihaan","IT","8.26","true"}};
    Row r4={{"104","Reben","EXTC","9.30","true"}};

    assert(table->insert(r1)==true);
    assert(table->insert(r2)==true);
    assert(table->insert(r3)==true);
    assert(table->insert(r4)==true);

    // ---------- Duplicate Primary Key ----------
    Row duplicate={{"101","Someone","IT","10.00","true"}};
    assert(table->insert(duplicate)==false);

    // Original row must still exist
    

Row latest = table->latest("101");

cout<<"Values size = "<<latest.values.size()<<'\n';

for(auto &x:latest.values)
    cout<<x<<" | ";

cout<<'\n';
    assert(latest.values[1]=="Swayam");

    // ---------- Invalid Integer ----------
    Row invalidPK={{"abc","Someone","IT","8.50","true"}};
    assert(table->insert(invalidPK)==false);

    // ---------- Invalid Float ----------
    Row invalidCGPA={{"105","Aryan","CS","abc","true"}};
    assert(table->insert(invalidCGPA)==false);

    // ---------- Invalid Bool ----------
    Row invalidBool={{"106","Harsh","IT","8.90","yes"}};
    assert(table->insert(invalidBool)==false);

    // ---------- Missing Column ----------
    Row missing={{"107","Yash","IT","8.50"}};
    assert(table->insert(missing)==false);

    // ---------- Extra Column ----------
    Row extra={{"108","Aman","IT","8.50","true","extra"}};
    assert(table->insert(extra)==false);

    // ---------- Long String ----------
    Row longName={{
        "109",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDE",
        "IT",
        "8.50",
        "true"
    }};
    assert(table->insert(longName)==false);

    // ---------- Empty Primary Key ----------
    Row emptyPK={{"","ABC","IT","8.50","true"}};
    assert(table->insert(emptyPK)==false);

    // ---------- Empty Name ----------
    Row emptyName={{"110","","IT","8.50","true"}};
    assert(table->insert(emptyName)==true);

    latest=table->latest("110");
    assert(latest.values[1]=="");

    // ---------- Boundary String ----------
    Row boundary={{
        "111",
        "ABCDEFGHIJKLMNOPQRSTUVWX123456",
        "IT",
        "8.50",
        "true"
    }};

    if(boundary.values[1].size()==30)
        assert(table->insert(boundary)==true);

    // ---------- Duplicate Again ----------
    assert(table->insert(r1)==false);
    assert(table->insert(r2)==false);
    assert(table->insert(r3)==false);
    assert(table->insert(r4)==false);

    // ---------- Existing Data Intact ----------
    latest=table->latest("102");
    assert(latest.values[0]=="102");
    assert(latest.values[1]=="Darshan");

    latest=table->latest("103");
    assert(latest.values[0]=="103");
    assert(latest.values[1]=="Vihaan");

    latest=table->latest("104");
    assert(latest.values[0]=="104");
    assert(latest.values[1]=="Reben");

    cout<<"\n====================================\n";
    cout<<"Insert tests passed successfully!\n";
    cout<<"====================================\n";

    return 0;
}