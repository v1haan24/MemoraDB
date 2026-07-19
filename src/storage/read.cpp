#include "table.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
using namespace std;

Row Table::readRow(uint64_t offset){
    fstream file("data/"+string(meta.name)+".db",ios::binary|ios::in);
    if(!file){
        cerr<<"Failed to open table file '"<<meta.name<<"'.\n";
        return {};
    }
    file.seekg(offset+rhsz,ios::beg);
    return serializer.readPayload(file,meta);
}

Row Table::latest(const string& pk){
    if(!history.contains(pk)){cerr << "No row found.\n"; return {}; }
    return readRow(history.latest(pk).offset);
}

//AI
void Table::printDatabase(){
    fstream file("data/"+string(meta.name)+".db",ios::binary|ios::in);
    if(!file){cerr<<"Failed to open table '"<<meta.name<<"'.\n"; return;}

    TableMeta temp;
    readBinary(file,temp.metadataSize);
    file.read(temp.name,tns);
    readBinary(file,temp.rowCount);
    readBinary(file,temp.payloadSize);
    readBinary(file,temp.columnCount);

    for(int i=0;i<temp.columnCount;i++){
        ColMeta col;
        serializer.readColumn(file,col);
        temp.columns.push_back(col);
    }

    cout<<"\n========== TABLE ==========\n";
    cout<<"Name          : "<<temp.name<<'\n';
    cout<<"Rows          : "<<temp.rowCount<<'\n';
    cout<<"Columns       : "<<temp.columnCount<<'\n';
    cout<<"Payload Size  : "<<temp.payloadSize<<'\n';
    cout<<"Metadata Size : "<<temp.metadataSize<<"\n\n";

    cout<<left
        <<setw(20)<<"Name"
        <<setw(10)<<"Type"
        <<setw(8)<<"Size"
        <<setw(8)<<"Offset"
        <<setw(5)<<"PK"<<'\n';

    for(auto &col:temp.columns){
        string type;
        if(col.type==INT) type="INT";
        else if(col.type==FLOAT) type="FLOAT";
        else if(col.type==STRING) type="STRING";
        else type="BOOL";

        cout<<left
            <<setw(20)<<col.name
            <<setw(10)<<type
            <<setw(8)<<col.size
            <<setw(8)<<col.offset
            <<setw(5)<<(col.isPK?"Y":"N")<<'\n';
    }

    cout<<"\n================== RECORDS ==================\n";

    cout<<left
        <<setw(10)<<"Offset"
        <<setw(22)<<"Timestamp"
        <<setw(8)<<"Del";

    for(auto &col:temp.columns)
        cout<<setw(12)<<col.name;
    cout<<'\n';

    while(true){
        uint64_t offset=file.tellg();
        uint64_t timestamp;
        readBinary(file,timestamp);
        if(!file) break;
        bool deleted;
        readBinary(file,deleted);
        Row row=serializer.readPayload(file,temp);
        time_t sec=timestamp/1000;
        tm *t=localtime(&sec);
        char buf[25];
        strftime(buf,sizeof(buf),"%d-%m-%Y %H:%M:%S",t);
        cout<<left
            <<setw(10)<<offset
            <<setw(22)<<buf
            <<setw(8)<<(deleted?"Y":"N");
        for(auto &v:row.values)
            cout<<setw(12)<<v;  
        cout<<'\n';
    }

    file.close();
}