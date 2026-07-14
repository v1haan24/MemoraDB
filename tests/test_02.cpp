#include <iostream>
#include <cassert>
#include "../src/catalog/catalog.h"
#include "../src/storage/table.h"
using namespace std;

int main(){
    // Run test_01.cpp first.

    Catalog catalog;
    TableMeta student=catalog.readMetadata("Student.db");

    cout<<"metadataSize = "<<student.metadataSize<<endl;
    cout<<"columnCount = "<<student.columnCount<<endl;
    cout<<"payloadSize = "<<student.payloadSize<<endl;

    Table table(student);

    // ---------- Recovery ----------
    assert(table.history["101"].size()==3);
    assert(table.history["102"].size()==1);
    assert(table.history["103"].size()==1);
    assert(table.history["104"].size()==1);

    Row d1={{"101","A","IT","8","true"}};
    Row d2={{"102","A","IT","8","true"}};
    Row d3={{"103","A","IT","8","true"}};
    Row d4={{"104","A","IT","8","true"}};

    Row r5={{"105","Aryan","CS","8.55","true"}};
    Row r6={{"106","Yash","IT","9.10","true"}};
    Row r7={{"107","Meet","AIDS","8.87","false"}};
    Row r8={{"108","Rohan","EXTC","7.65","true"}};

    // Duplicate inserts after recovery
    assert(table.insert(d1)==false);
    assert(table.insert(d2)==false);
    assert(table.insert(d3)==false);
    assert(table.insert(d4)==false);

    // Fresh inserts
    assert(table.insert(r5)==true);
    assert(table.insert(r6)==true);
    assert(table.insert(r7)==true);
    assert(table.insert(r8)==true);

    // Duplicate inserts
    assert(table.insert(r5)==false);
    assert(table.insert(r6)==false);
    assert(table.insert(r7)==false);
    assert(table.insert(r8)==false);

    // Updates
    Row u5={{"105","Aryan","CS","9.75","false"}};
    Row u6={{"106","Yash","IT","9.90","true"}};

    assert(table.update(u5)==true);
    assert(table.update(u6)==true);

    // Duplicate updates on deleted/missing rows
    assert(table.update(d1)==false);

    Row missing={{"999","ABC","IT","8.00","true"}};
    assert(table.update(missing)==false);

    // Delete
    assert(table.deleteRow("101")==false);
    assert(table.deleteRow("999")==false);

    // Recovered History
    cout<<"\nRecovered History\n";
    for(const auto &p:table.history){
        cout<<p.first<<" -> ";
        for(const auto &v:p.second)
            cout<<"["<<v.timestamp<<", "<<v.offset<<"] ";
        cout<<'\n';
    }

    cout<<'\n';
    table.printDatabase();

    // Read latest version of 105
    uint64_t offset=table.history["105"].back().offset;
    Row recovered=table.readRow(offset);

    assert(recovered.values[0]=="105");
    assert(recovered.values[1]=="Aryan");
    assert(recovered.values[2]=="CS");
    assert(recovered.values[3]=="9.750000");
    assert(recovered.values[4]=="false");

    // Latest
    Row latest=table.latest("105");

    assert(latest.values[0]=="105");
    assert(latest.values[1]=="Aryan");
    assert(latest.values[2]=="CS");
    assert(latest.values[3]=="9.750000");
    assert(latest.values[4]=="false");

    // History sizes
    assert(table.history["101"].size()==3);
    assert(table.history["105"].size()==2);
    assert(table.history["106"].size()==2);
    assert(table.history["107"].size()==1);
    assert(table.history["108"].size()==1);

    cout<<"\nRecovery test passed!\n";
    return 0;
}