#include <iostream>
#include <thread>
#include <chrono>
#include "../src/catalog/catalog.h"
#include "../src/storage/table.h"
#include <filesystem>
using namespace std;
using namespace std::chrono;

void wait(){
    this_thread::sleep_for(milliseconds(50));
}
uint64_t checkpoint(){
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}
void heading(const string &s){
    cout<<"\n";
    cout<<"====================================================\n";
    cout<<s<<'\n';
    cout<<"====================================================\n";
}
int main(){
    filesystem::remove_all("data/Students");
    Catalog catalog;
    TableMeta meta;
    strcpy(meta.name,"Students");
    meta.columns={
        ColMeta("Roll",STRING,true,10),
        ColMeta("Name",STRING,false,20),
        ColMeta("Branch",STRING,false,20),
        ColMeta("Fee",STRING,false,20),
        ColMeta("Concession",STRING,false,20),
        ColMeta("Hostel",STRING,false,10),
        ColMeta("Club",STRING,false,20)
    };
    if(!catalog.createTable(meta)){
        cout<<"Table already exists.\n";
    }
    Table *table=catalog.getTable("Students");
    if(table==nullptr){
        cout<<"Unable to open table.\n";
        return 0;
    }
    heading("INSERTING INITIAL RECORDS (T1)");
    table->insert({{"26","Vihaan","IT","Pending","Not Issued","false","CP Club"}});
    wait();
    table->insert({{"21","Swayam","CS","Pending","Not Issued","false","CP Club"}});
    wait();
    table->insert({{"36","Darshan","EXTC","Paid","Issued","true","ProjX"}});
    wait();
    table->insert({{"31","Reuben","IT","Paid","Issued","true","Techno"}});
    wait();
    uint64_t T1=checkpoint();
    cout<<"Checkpoint T1 Created\n";
    cout<<"T1 = "<<T1<<"\n";
    heading("UPDATES (T2)");
    table->update({{"26","Vihaan","IT","Paid","Issued","false","CP Club"}});
    wait();
    table->update({{"21","Swayam","CS","Paid","Issued","true","CP Club"}});
    wait();
    table->update({{"36","Darshan","EXTC","Paid","Issued","true","CP Club"}});
    wait();
    uint64_t T2=checkpoint();
    cout<<"Checkpoint T2 Created\n";
    cout<<"T2 = "<<T2<<"\n";
    heading("UPDATES (T3)");
    table->update({{"26","Vihaan","Electronics","Paid","Issued","true","ProjX"}});
    wait();
    table->update({{"31","Reuben","IT","Paid","Renewed","false","Techno"}});
    wait();
    uint64_t T3=checkpoint();
    cout<<"Checkpoint T3 Created\n";
    cout<<"T3 = "<<T3<<"\n";
    heading("UPDATES (T4)");
    table->update({{"26","Vihaan","Electronics","Paid","Renewed","true","ProjX"}});
    wait();
    table->update({{"36","Darshan","EXTC","Paid","Renewed","false","CP Club"}});
    wait();
    uint64_t T4=checkpoint();
    cout<<"Checkpoint T4 Created\n";
    cout<<"T4 = "<<T4<<"\n";
    heading("DATABASE");
    table->printDatabase();
    heading("LATEST (Vihaan)");
    print(table->latest("26"));
    heading("AS OF (Vihaan @ T2)");
    print(table->selectAsOf("26",T2));
    heading("HISTORY (Vihaan)");
    print(table->showHistory("26"));
    heading("BETWEEN (Vihaan : T2 -> T4)");
    print(table->selectBetween("26",T2,T4));
    heading("SNAPSHOT (T3)");
    vector<Record> snap=table->snapshot(T3);
    print(snap);
    heading("COMPARE (Vihaan : T1 vs T4)");
    print(table->compare("26",T1,T4));
    heading("EVOLUTION (Vihaan)");
    print(table->evolution("26",T1,T4));
    heading("ROW ROLLBACK (Vihaan -> T2)");
    if(table->rollback("26",T2)){
        cout<<"Rollback Successful\n";
        cout<<"\nLatest Record\n";
        print(table->latest("26"));
    }
    else{
        cout<<"Rollback Failed\n";
    }
    wait();
    heading("TABLE ROLLBACK (Entire Table -> T2)");
    if(table->rollback(T2)){
        cout<<"Table Rollback Successful\n";
    }
    else{
        cout<<"Table Rollback Failed\n";
    }
    wait();
    heading("LATEST AFTER TABLE ROLLBACK");
    cout<<"\nVihaan\n";
    print(table->latest("26"));
    cout<<"\nSwayam\n";
    print(table->latest("21"));
    cout<<"\nDarshan\n";
    print(table->latest("36"));
    cout<<"\nReuben\n";
    print(table->latest("31"));
    heading("SNAPSHOT AFTER ROLLBACK");
    print(table->snapshot(table->latest("26").timestamp));
    heading("SHOWCASE COMPLETE");
    cout<<"\nTemporal Queries Demonstrated\n";
    cout<<"1. Latest\n";
    cout<<"2. As Of\n";
    cout<<"3. History\n";
    cout<<"4. Between\n";
    cout<<"5. Snapshot\n";
    cout<<"6. Compare\n";
    cout<<"7. Evolution\n";
    cout<<"8. Row Rollback\n";
    cout<<"9. Table Rollback\n";
    return 0;
}