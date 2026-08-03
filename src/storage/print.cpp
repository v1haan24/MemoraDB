#include "table.h"
#include <iostream>

void print(const Row& row){
    for(int i=0;i<row.values.size();i++){
        std::cout<<row.values[i];
        if(i+1!=row.values.size()) std::cout<<" | ";
    }
    std::cout<<'\n';
}
void print(const Record& record){
    std::cout<<"Timestamp : "<<record.timestamp<<'\n';
    std::cout<<"Deleted  : "<<(record.deleted?"true":"false")<<'\n';
    std::cout<<"Row:"<<'\n';
    print(record.row);
}
void print(const std::vector<Row>& rows){
    if(rows.empty()){std::cout<<"No rows found.\n"; return;}
    for(const auto& row:rows){print(row); std::cout<<'\n';}
}
void print(const std::vector<Record>& records){
    if(records.empty()){std::cout<<"No records found.\n";return;}
    for(const auto& record:records){print(record); std::cout<<'\n';}
}
void print(const Difference& diff){
    if(diff.timestamp!=0)
    std::cout<<"Timestamp : "<<diff.timestamp<<'\n';
    std::cout<<"Column    : "<<diff.column<<'\n';
    std::cout<<"Before    : "<<diff.before<<'\n';
    std::cout<<"After     : "<<diff.after<<'\n';
}
void print(const std::vector<Difference>& diffs){
    if(diffs.empty()){std::cout<<"No differences found.\n";return;}
    for(const auto& diff:diffs){ print(diff);std::cout<<'\n';}
}


void Table::printDatabase(){
    file.clear();
    file.seekg(0,std::ios::beg);
    
    TableMeta temp;
    readBinary(file,temp.metadataSize);
    file.read(temp.name,tns);
    readBinary(file,temp.payloadSize);
    readBinary(file,temp.columnCount);
    if(!file){ std::cerr<<"Failed to read table metadata.\n"; return;}

    for(int i=0;i<temp.columnCount;i++){
        ColMeta col;
        readColumn(file,col);
        temp.columns.push_back(col);
    }

    std::cout<<"Table : "<<temp.name<<'\n';
    std::cout<<"Columns : "<<temp.columnCount<<'\n';
    std::cout<<"Payload Size : "<<temp.payloadSize<<'\n';
    std::cout<<"Metadata Size : "<<temp.metadataSize<<"\n\n";

    std::cout<<"Columns\n";
    for(const auto& col:temp.columns){
        std::cout<<"  "<<col.name
            <<" | "
            <<(col.type==INT?"INT":
               col.type==FLOAT?"FLOAT":
               col.type==STRING?"STRING":"BOOL")
            <<" | Size="<<col.size
            <<" | Offset="<<col.offset
            <<" | PK="<<(col.isPK?"true":"false")
            <<'\n';
    }

    std::cout<<"\nRecords\n";

    while(true){
        uint64_t offset=static_cast<uint64_t>(file.tellg());
        Record record;
        readBinary(file,record.timestamp);
        if(!file) break;
        readBinary(file,record.deleted);
        if(!file){ std::cerr<<"Corrupted record.\n"; break;}
        record.row=readPayload(file,temp);
        if(!file){ std::cerr<<"Corrupted record payload.\n"; break;}
        std::cout<<"\nOffset : "<<offset<<'\n';
        print(record);
    }

    
}
