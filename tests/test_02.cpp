#include <iostream>
#include <cassert>
#include "../src/catalog/catalog.h"
#include "../src/storage/table.h"
using namespace std;

int main(){
    //run test_01.cpp first always.
    Catalog catalog;
    TableMeta student = catalog.readMetadata("Student.db");
    cout<<"metadataSize = "<<student.metadataSize<<endl;
    cout<<"columnCount = "<<student.colCount<<endl;
    cout<<"payloadSize = "<<student.payloadSize<<endl;

    Table table(student);
    Row duplicate1={{"101","Someone","IT","10.00","true"}};
    Row duplicate2={{"102","Someone","CS","8.00","true"}};

    Row d1={{"101","A","IT","8","true"}};
    Row d2={{"102","A","IT","8","true"}};
    Row d3={{"103","A","IT","8","true"}};
    Row d4={{"104","A","IT","8","true"}};
    Row r5={{"105","Aryan","CS","8.55","true"}};
    Row r6={{"106","Yash","IT","9.10","true"}};
    Row r7={{"107","Meet","AIDS","8.87","false"}};
    Row r8={{"108","Rohan","EXTC","7.65","true"}};

    assert(table.insert(d1)==false);
    assert(table.insert(d2)==false);
    assert(table.insert(d3)==false);
    assert(table.insert(d4)==false);

    assert(table.insert(r5)==true);
    assert(table.insert(r6)==true);
    assert(table.insert(r7)==true);
    assert(table.insert(r8)==true);

    assert(table.insert(r5)==false);
    assert(table.insert(r6)==false);
    assert(table.insert(r7)==false);
    assert(table.insert(r8)==false);

    cout<<"\nRecovered Map\n";
    for(const auto &p:table.history){
        cout << p.first << " -> ";
        for(const auto &v : p.second){
            cout<<"["<<v.timestamp<<", "<<v.offset<<"] ";
        }

        cout<<'\n';
    }

    cout<<"Recovery test passed!\n";
    return 0;
}