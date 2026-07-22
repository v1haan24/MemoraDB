#include <iostream>
#include <cassert>
#include "../src/catalog/catalog.h"

using namespace std;

int main(){

    Catalog catalog;

    Table* table=catalog.getTable("Student");
    assert(table!=nullptr);

    vector<Record> history=table->showHistory("101");
    assert(history.size()==4);

    uint64_t t1=history[0].timestamp;
    uint64_t t2=history[1].timestamp;
    uint64_t t3=history[2].timestamp;
    uint64_t t4=history[3].timestamp;

    cout<<"\n========== COMPARE t1 -> t3 ==========\n";
    print(table->compare("101",t1,t3));

    cout<<"\n========== EVOLUTION t1 -> t4 ==========\n";
    print(table->evolution("101",t1,t4));

    cout<<"\n========== ROLLBACK ROW (t1) ==========\n";
    assert(table->rollback("101",t1));

    print(table->latest("101"));

    cout<<"\nHistory After Rollback\n";
    print(table->showHistory("101"));

    cout<<"\n========== TABLE ROLLBACK (t2) ==========\n";
    assert(table->rollback(t2));

    cout<<"\nSnapshot After Table Rollback\n";
    print(table->snapshot(UINT64_MAX));

    cout<<"\n========== DATABASE ==========\n";
    table->printDatabase();

    cout<<"\n====================================\n";
    cout<<"Advanced Temporal Tests Passed Successfully!\n";
    cout<<"====================================\n";

    return 0;
}