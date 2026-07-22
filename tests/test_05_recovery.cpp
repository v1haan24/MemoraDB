#include <iostream>
#include <cassert>
#include <cstdio>

#include "../src/catalog/catalog.h"

using namespace std;

int main(){

    remove("data/Student.db");

    // ---------- Create Database ----------
    {
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

        Row r1={{"101","Swayam","IT","7.95","true"}};
        Row r2={{"102","Darshan","CS","7.70","true"}};
        Row r3={{"103","Vihaan","IT","8.26","true"}};

        assert(table->insert(r1));
        assert(table->insert(r2));
        assert(table->insert(r3));

        Row u1={{"101","Swayam","IT","9.50","false"}};
        Row u2={{"102","Darshan","CS","8.80","true"}};

        assert(table->update(u1));
        assert(table->update(u2));

        assert(table->deleteRow("103"));
    }

    // ---------- Restart ----------
    Catalog catalog;

    Table* table=catalog.getTable("Student");

    assert(table!=nullptr);

    // ---------- Latest After Recovery ----------
    Record latest=table->latest("101");

    assert(latest.row.values[0]=="101");
    assert(latest.row.values[1]=="Swayam");
    assert(latest.row.values[2]=="IT");
    assert(latest.row.values[3]=="9.500000");
    assert(latest.row.values[4]=="false");

    latest=table->latest("102");

    assert(latest.row.values[0]=="102");
    assert(latest.row.values[1]=="Darshan");
    assert(latest.row.values[3]=="8.800000");
    assert(latest.row.values[4]=="true");

    // ---------- Deleted Row ----------
    assert(table->update({
        {"103","Vihaan","IT","9.99","true"}
    })==false);

    // ---------- Duplicate Inserts ----------
    Row d1={{"101","ABC","IT","1.00","true"}};
    Row d2={{"102","ABC","IT","1.00","true"}};

    assert(table->insert(d1)==false);
    assert(table->insert(d2)==false);

    // ---------- Fresh Inserts ----------
    Row r4={{"104","Reben","EXTC","9.20","true"}};
    Row r5={{"105","Aryan","CS","8.55","true"}};

    assert(table->insert(r4));
    assert(table->insert(r5));

    latest=table->latest("104");

    assert(latest.row.values[0]=="104");
    assert(latest.row.values[1]=="Reben");

    latest=table->latest("105");

    assert(latest.row.values[0]=="105");
    assert(latest.row.values[1]=="Aryan");

    cout<<"\n====================================\n";
    cout<<"Recovery tests passed successfully!\n";
    cout<<"====================================\n";

    return 0;
}