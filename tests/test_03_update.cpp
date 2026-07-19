#include <iostream>
#include <cassert>
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

    assert(catalog.createTable(student)==true);

    Table* table=catalog.getTable("Student");
    assert(table!=nullptr);

    // ---------- Initial Inserts ----------
    Row r1={{"101","Swayam","IT","7.95","true"}};
    Row r2={{"102","Darshan","CS","7.70","true"}};
    Row r3={{"103","Vihaan","IT","8.26","true"}};

    assert(table->insert(r1));
    assert(table->insert(r2));
    assert(table->insert(r3));

    // ---------- Valid Update ----------
    Row u1={{"101","Swayam","IT","9.50","false"}};

    assert(table->update(u1)==true);

    Row latest=table->latest("101");

    assert(latest.values[0]=="101");
    assert(latest.values[1]=="Swayam");
    assert(latest.values[2]=="IT");
    assert(latest.values[3]=="9.500000");
    assert(latest.values[4]=="false");

    // ---------- Update Again ----------
    Row u2={{"101","Swayam Jain","IT","9.90","true"}};

    assert(table->update(u2)==true);

    latest=table->latest("101");

    assert(latest.values[1]=="Swayam Jain");
    assert(latest.values[3]=="9.900000");
    assert(latest.values[4]=="true");

    // ---------- Missing PK ----------
    Row missing={{"999","ABC","IT","8.00","true"}};

    assert(table->update(missing)==false);

    // ---------- Invalid Integer ----------
    Row invalidPK={{"abc","ABC","IT","8.00","true"}};

    assert(table->update(invalidPK)==false);

    // ---------- Invalid Float ----------
    Row invalidFloat={{"101","ABC","IT","xyz","true"}};

    assert(table->update(invalidFloat)==false);

    // ---------- Invalid Bool ----------
    Row invalidBool={{"101","ABC","IT","8.00","yes"}};

    assert(table->update(invalidBool)==false);

    // ---------- Missing Column ----------
    Row missingCol={{"101","ABC","IT","8.00"}};

    assert(table->update(missingCol)==false);

    // ---------- Extra Column ----------
    Row extraCol={{"101","ABC","IT","8.00","true","extra"}};

    assert(table->update(extraCol)==false);

    // ---------- Long String ----------
    Row longName={{
        "101",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDE",
        "IT",
        "8.00",
        "true"
    }};

    assert(table->update(longName)==false);

    // ---------- Verify Previous Version Intact ----------
    latest=table->latest("101");

    assert(latest.values[1]=="Swayam Jain");
    assert(latest.values[3]=="9.900000");
    assert(latest.values[4]=="true");

    // ---------- Update Other Row ----------
    Row u3={{"102","Darshan","CS","8.80","false"}};

    assert(table->update(u3));

    latest=table->latest("102");

    assert(latest.values[0]=="102");
    assert(latest.values[1]=="Darshan");
    assert(latest.values[3]=="8.800000");
    assert(latest.values[4]=="false");

    // ---------- Duplicate Updates ----------
    assert(table->update(u3));
    assert(table->update(u3));

    latest=table->latest("102");

    assert(latest.values[3]=="8.800000");
    assert(latest.values[4]=="false");

    cout<<"\n====================================\n";
    cout<<"Update tests passed successfully!\n";
    cout<<"====================================\n";

    return 0;
}