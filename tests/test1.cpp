#include <iostream>
#include <cassert>
#include <cstdio>

#include "../src/catalog/catalog.h"
#include "../src/storage/table.h"

using namespace std;

int main() {

    // Remove previous test files
    remove("Employee.db");
    remove("Employee_data.dat");

    DatabaseCatalog catalog;

    // ---------------- Table Definition ----------------
    TableStruct employee;
    strcpy(employee.tblName, "Employee");

    employee.colList.push_back(ColumnInfo("empId", INT, true));
    employee.colList.push_back(ColumnInfo("name", STRING, false, 30));
    employee.colList.push_back(ColumnInfo("department", STRING, false, 15));
    employee.colList.push_back(ColumnInfo("salary", FLOAT, false));
    employee.colList.push_back(ColumnInfo("active", BOOL, false));

    // ---------------- Catalog Tests ----------------
    assert(catalog.makeNewTable(employee) == true);
    assert(catalog.makeNewTable(employee) == false);   // Duplicate table

    employee = catalog.loadTable("Employee.db");

    // ---------------- Storage Engine ----------------
    Table table(employee);

    // ---------------- Valid Records ----------------
    DataRow r1{{"101", "Anonymous1", "IT", "7.95", "true"}};
    DataRow r2{{"102", "Anonymous2", "HR", "7.70", "true"}};
    DataRow r3{{"103", "Anonymous3", "IT", "8.26", "true"}};

    assert(table.insertRecord(r1) == true);
    assert(table.insertRecord(r2) == true);
    assert(table.insertRecord(r3) == true);

    assert(table.getRowCount() == 3);

    // ---------------- Duplicate PK ----------------
    DataRow duplicate{{"101", "Anonymous4", "IT", "9.50", "true"}};
    assert(table.insertRecord(duplicate) == false);

    // ---------------- Invalid Integer ----------------
    DataRow badInt{{"abc", "Anonymous5", "IT", "8.50", "true"}};
    assert(table.insertRecord(badInt) == false);

    // ---------------- Invalid Float ----------------
    DataRow badFloat{{"104", "Anonymous6", "IT", "abc", "true"}};
    assert(table.insertRecord(badFloat) == false);

    // ---------------- Invalid Bool ----------------
    DataRow badBool{{"105", "Anonymous7", "IT", "8.90", "yes"}};
    assert(table.insertRecord(badBool) == false);

    // ---------------- Missing Column ----------------
    DataRow shortRow{{"106", "Anonymous8"}};
    assert(table.insertRecord(shortRow) == false);

    // ---------------- Extra Column ----------------
    DataRow extraRow{{"107", "Anonymous9", "IT", "8.90", "true", "extra"}};
    assert(table.insertRecord(extraRow) == false);

    // ---------------- Long String ----------------
    DataRow longName{{"108",
                      "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDE",
                      "IT",
                      "8.90",
                      "true"}};

    assert(table.insertRecord(longName) == false);

    cout << "\n====================================\n";
    cout << "All tests passed successfully!\n";
    cout << "====================================\n";

    return 0;
}