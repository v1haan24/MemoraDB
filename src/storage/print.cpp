#include "table.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
using namespace std;

void print(const Row& row){
    for(int i=0;i<row.values.size();i++){
        cout<<row.values[i];
        if(i+1!=row.values.size()) cout<<" | ";
    }
    cout<<'\n';
}
void print(const Record& record){
    cout<<"Timestamp : "<<record.timestamp<<'\n';
    cout<<"Deleted  : "<<(record.deleted?"true":"false")<<'\n';
    cout<<"Row:"<<'\n';
    print(record.row);
}
void print(const vector<Row>& rows){
    if(rows.empty()){cout<<"No rows found.\n"; return;}
    for(const auto& row:rows){print(row); cout<<'\n';}
}
void print(const vector<Record>& records){
    if(records.empty()){cout<<"No records found.\n";return;}
    for(const auto& record:records){print(record); cout<<'\n';}
}
void print(const Difference& diff){
    if(diff.timestamp!=0)
    cout<<"Timestamp : "<<diff.timestamp<<'\n';
    cout<<"Column    : "<<diff.column<<'\n';
    cout<<"Before    : "<<diff.before<<'\n';
    cout<<"After     : "<<diff.after<<'\n';
}
void print(const vector<Difference>& diffs){
    if(diffs.empty()){cout<<"No differences found.\n";return;}
    for(const auto& diff:diffs){ print(diff);cout<<'\n';}
}


void Table::printDatabase(){
    fstream file("data/"+string(meta.name)+".db",ios::binary|ios::in);
    if(!file){
        cerr<<"Failed to open table '"<<meta.name<<"'.\n";
        return;
    }

    TableMeta temp;
    readBinary(file,temp.metadataSize);
    file.read(temp.name,tns);
    readBinary(file,temp.rowCount);
    readBinary(file,temp.payloadSize);
    readBinary(file,temp.columnCount);

    for(int i=0;i<temp.columnCount;i++){
        ColMeta col;
        readColumn(file,col);
        temp.columns.push_back(col);
    }

    cout<<"Table : "<<temp.name<<'\n';
    cout<<"Rows : "<<temp.rowCount<<'\n';
    cout<<"Columns : "<<temp.columnCount<<'\n';
    cout<<"Payload Size : "<<temp.payloadSize<<'\n';
    cout<<"Metadata Size : "<<temp.metadataSize<<"\n\n";

    cout<<"Columns\n";
    for(const auto& col:temp.columns){
        cout<<"  "<<col.name
            <<" | "
            <<(col.type==INT?"INT":
               col.type==FLOAT?"FLOAT":
               col.type==STRING?"STRING":"BOOL")
            <<" | Size="<<col.size
            <<" | Offset="<<col.offset
            <<" | PK="<<(col.isPK?"true":"false")
            <<'\n';
    }

    cout<<"\nRecords\n";

    while(true){
        uint64_t offset=file.tellg();

        Record record;
        readBinary(file,record.timestamp);
        if(!file) break;

        readBinary(file,record.deleted);
        record.row=readPayload(file,temp);

        cout<<"\nOffset : "<<offset<<'\n';
        print(record);
    }

    file.close();
}