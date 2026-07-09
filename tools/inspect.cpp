#include<iostream>
#include<fstream>
#include<vector>
#include "../src/types.h"
#include "../src/serializer.h"
using namespace std;

int main(){
    ifstream file("data/Employee.db",ios::binary);
    if(!file){
        cout<<"Cannot open file\n";
        return 0;
    }

    TableMeta meta;
    readBinary(file,meta.metadataSize);
    file.read(meta.name,tns);
    readBinary(file,meta.rowCount);
    readBinary(file,meta.payloadSize);
    readBinary(file,meta.columnCount);

    for(int i=0;i<meta.columnCount;i++){
        ColMeta col;
        readColumn(file,col);
        meta.columns.push_back(col);
    }

    cout<<"Table : "<<meta.name<<endl;
    cout<<"Rows  : "<<meta.rowCount<<"\n\n";

    file.seekg(meta.metadataSize,ios::beg);
    for(int i=0;i<meta.rowCount;i++){
        uint64_t t;
        bool del;

        readBinary(file,t);
        readBinary(file,del);

        cout<<"Record "<<i+1<<endl;
        cout<<"Timestamp : "<<t<<endl;
        cout<<"Deleted   : "<<del<<endl;

        for(int j=0;j<meta.columnCount;j++){
            cout<<meta.columns[j].name<<" : ";

            if(meta.columns[j].type==INT){
                int x;
                readBinary(file,x);
                cout<<x;
            }

            else if(meta.columns[j].type==FLOAT){
                float x;
                readBinary(file,x);
                cout<<x;
            }

            else if(meta.columns[j].type==BOOL){
                bool x;
                readBinary(file,x);
                cout<<x;
            }

            else{
                vector<char> temp(meta.columns[j].size,'\0');
                file.read(temp.data(),meta.columns[j].size);
                cout<<temp.data();
            }

            cout<<endl;
        }

        cout<<"--------------------------\n";
    }

    file.close();
}