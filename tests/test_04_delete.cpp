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

    assert(catalog.createTable(student));

    Table* table=catalog.getTable("Student");
    assert(table!=nullptr);

    // ---------- Inserts ----------
    Row r1={{"101","Swayam","IT","7.95","true"}};
    Row r2={{"102","Darshan","CS","7.70","true"}};
    Row r3={{"103","Vihaan","IT","8.26","true"}};

    assert(table->insert(r1));
    assert(table->insert(r2));
    assert(table->insert(r3));

    // ---------- Valid Delete ----------
    assert(table->deleteRow("101")==true);

    // ---------- Double Delete ----------
    assert(table->deleteRow("101")==false);

    // ---------- Missing PK ----------
    assert(table->deleteRow("999")==false);

    // ---------- Cannot Update Deleted ----------
    Row updateDeleted={{"101","Swayam","IT","9.50","true"}};
    assert(table->update(updateDeleted)==false);

    // ---------- Cannot Insert Same PK ----------
    assert(table->insert(r1)==false);

    // ---------- Other Rows Still Exist ----------
    Record latest=table->latest("102");

    assert(latest.row.values[0]=="102");
    assert(latest.row.values[1]=="Darshan");
    assert(latest.row.values[2]=="CS");
    assert(latest.row.values[3]=="7.700000");
    assert(latest.row.values[4]=="true");

    latest=table->latest("103");

    assert(latest.row.values[0]=="103");
    assert(latest.row.values[1]=="Vihaan");

    // ---------- Delete Remaining ----------
    assert(table->deleteRow("102"));
    assert(table->deleteRow("103"));

    // ---------- Delete Again ----------
    assert(table->deleteRow("102")==false);
    assert(table->deleteRow("103")==false);

    // ---------- Cannot Update Deleted ----------
    Row u2={{"102","Darshan","CS","8.50","false"}};
    Row u3={{"103","Vihaan","IT","9.00","true"}};

    assert(table->update(u2)==false);
    assert(table->update(u3)==false);

    // ---------- Cannot Delete Missing ----------
    assert(table->deleteRow("500")==false);

    cout<<"\n====================================\n";
    cout<<"Delete tests passed successfully!\n";
    cout<<"====================================\n";

    return 0;
}