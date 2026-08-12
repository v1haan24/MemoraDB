#include "table.h"
#include <chrono>
#include <cstring>
#include <filesystem>
#include <mutex>

void writeString(std::ostream& file,const std::string& s){
    uint32_t len=(uint32_t)s.size();
    writeBinary(file,len);
    if(len) file.write(s.data(),len);
}

void readString(std::istream& file,std::string& s){
    uint32_t len=0;
    readBinary(file,len);
    s.resize(len);
    if(len) file.read(&s[0],len);
}

bool writeMetadata(std::fstream& out,const TableMeta& meta){
    writeBinary(out,meta.metadataSize);
    out.write(meta.name,tns);
    writeBinary(out,meta.payloadSize);
    writeBinary(out,meta.columnCount);
    for(const auto& col:meta.columns) writeColumn(out,col);
    return out.good();
}

void writeColumn(std::ostream& file,const ColMeta& col){
    file.write(col.name,cns);
    writeBinary(file,col.type);
    writeBinary(file,col.size);
    writeBinary(file,col.offset);
    writeBinary(file,col.isPK);
}

void readColumn(std::istream& file,ColMeta& col){
    file.read(col.name,cns);
    readBinary(file,col.type);
    readBinary(file,col.size);
    readBinary(file,col.offset);
    readBinary(file,col.isPK);
}

uint64_t writeHeader(std::fstream& file,bool deleted){
    uint64_t timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    writeBinary(file,timestamp);
    writeBinary(file,deleted);
    return timestamp;
}

void writePayload(std::fstream& file,const TableMeta& meta,const Row& row){
    for(int i=0;i<meta.columnCount;i++){
        if(meta.columns[i].type==INT){
            int x=stoi(row.values[i]);
            writeBinary(file,x);
        }
        else if(meta.columns[i].type==FLOAT){
            float x=stof(row.values[i]);
            writeBinary(file,x);
        }
        else if(meta.columns[i].type==BOOL){
            bool x=(row.values[i]=="true");
            writeBinary(file,x);
        }
        else if(meta.columns[i].type==STRING){
            std::string temp(meta.columns[i].size,'\0');
            int len=row.values[i].size();
            if(len>meta.columns[i].size-1) len=meta.columns[i].size-1;
            memcpy(temp.data(),row.values[i].data(),len);
            file.write(temp.data(),meta.columns[i].size);
        }
    }
}

Row readPayload(std::fstream& file,const TableMeta& meta){
    Row row;
    for(int i=0;i<meta.columnCount;i++){
        if(meta.columns[i].type==INT){
            int x;
            readBinary(file,x);
            row.values.push_back(std::to_string(x));
        }
        else if(meta.columns[i].type==FLOAT){
            float x;
            readBinary(file,x);
            row.values.push_back(std::to_string(x));
        }
        else if(meta.columns[i].type==BOOL){
            bool x;
            readBinary(file,x);
            row.values.push_back(x?"true":"false");

        }
        else if(meta.columns[i].type==STRING){
            std::string temp(meta.columns[i].size,'\0');
            file.read(&temp[0], meta.columns[i].size);
            temp.resize(strnlen(temp.c_str(),meta.columns[i].size));
            row.values.push_back(temp);
        }
    }
    return row;
}


bool Table::writeQueue(std::string pk,uint64_t timestamp,const Row& row){
    std::string temp = "";
    for(int i = 0; i<meta.columnCount; i++){
        if(meta.columns[i].isSemantic){
            temp = temp + meta.columns[i].name + ": ";
            temp += row.values[i] + " ";
        }
    }

    if(temp.length()==0) return false; // no semantic columns on this table -> nothing to embed

    std::string dir = "data/"+std::string(meta.name)+"/queue";
    std::filesystem::create_directories(dir);
    std::string detPath = dir+"/temp_det.queue";
    std::string tasksPath = dir+"/temp_tasks.queue";

    // make sure both files exist first 
    { std::ofstream touch1(detPath, std::ios::app|std::ios::binary); }
    { std::ofstream touch2(tasksPath, std::ios::app); }

    std::fstream file1(detPath, std::ios::in | std::ios::out | std::ios::binary);
    std::fstream file2(tasksPath, std::ios::in | std::ios::out);
    if(!file1 || !file2){
        std::cerr<<"Failed to open queue files for writing.\n";
        return false;
    }

    static std::mutex queueMutex;
    std::lock_guard<std::mutex> lock(queueMutex);

    file1.seekp(0,std::ios::end);
    bool wasEmpty = (file1.tellp()==0); //Check if the file is empty
    if(wasEmpty){
        int c = 0;
        writeBinary(file1,c);
    }

    writeString(file1, pk); 
    writeBinary(file1, timestamp);

    file2.seekp(0,std::ios::end);
    file2<<temp<<"\n";

    int count;
    file1.seekg(0,std::ios::beg);
    readBinary(file1,count);
    count++;
    file1.seekp(0,std::ios::beg);
    writeBinary(file1,count);

    if(!file1 || !file2){
        std::cerr<<"Failed to write to queue files.\n";
        return false;
    }
    return true;
}