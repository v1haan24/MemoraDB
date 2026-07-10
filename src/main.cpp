#include <iostream>
#include <cassert>
#include "catalog/catalog.h"
#include "storage/table.h"

using namespace std;

int main() {
    DatabaseCatalog catalog;

    // 1. Setup the Catalog
    TableStruct emp;
    strcpy(emp.tblName, "Employee");
    emp.colList.push_back(ColumnInfo("empId", INT, true));
    emp.colList.push_back(ColumnInfo("name", STRING, false, 30));
    emp.colList.push_back(ColumnInfo("salary", FLOAT, false));
    emp.colList.push_back(ColumnInfo("isActive", BOOL, false));
    
    catalog.makeNewTable(emp);
    cout << "[CATALOG] Employee schema created/loaded.\n";

    // 2. Load the Table from Catalog into the Storage Engine
    TableStruct loadedMeta = catalog.loadTable("Employee.db");
    Table empTable(loadedMeta);

    // 3. Prepare test rows
    DataRow validRow{{"101", "VIVAAN", "95000.0", "true"}};
    DataRow crashRow{{"abc", "Naman", "xyz", "true"}}; 
    DataRow duplicateRow{{"101", "Imposter", "10000.0", "false"}};

    // 4. Assert Engine Safety
    cout << "[STORAGE] Running engine checks...\n";
    
    assert(empTable.insertRecord(crashRow) == false); 
    assert(empTable.insertRecord(validRow) == true);
    assert(empTable.insertRecord(duplicateRow) == false); 

    cout << "All modules integrated successfully. MemoraDB is active!\n";

    return 0;
}