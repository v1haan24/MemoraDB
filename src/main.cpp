#include <iostream>
#include "catalog.h"
#include "table.h"

using namespace std;

int main()
{
    Catalog catalog;

    TableMeta employee;
    strcpy(employee.name,"Employee");
    employee.columns.push_back({"id",INT,true});
    employee.columns.push_back({"name",STRING,false,21});
    employee.columns.push_back({"salary",FLOAT,false});
    employee.columns.push_back({"active",BOOL,false});

    catalog.createTable(employee);

    Table table(employee);

    Row r1;
    r1.values={"1","Alice","50000.5","true"};

    Row r2;
    r2.values={"2","Bob","75000","false"};

    Row r3;
    r3.values={"1","Charlie","90000","true"};   // Duplicate PK

    cout<<"Insert 1 : "<<table.insert(r1)<<endl;
    cout<<"Insert 2 : "<<table.insert(r2)<<endl;
    cout<<"Insert 3 : "<<table.insert(r3)<<endl;

    return 0;
}