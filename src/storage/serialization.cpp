#include "table.h"
#include <chrono>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <string>

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
    writeBinary(file,col.isSemantic);
}

void readColumn(std::istream& file,ColMeta& col){
    file.read(col.name,cns);
    readBinary(file,col.type);
    readBinary(file,col.size);
    readBinary(file,col.offset);
    readBinary(file,col.isPK);
    readBinary(file,col.isSemantic);
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

#include <filesystem>
#include <mutex>
#include <thread>
#include "../vector/embed_queue.h"
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
bool Table::writeQueue(std::string pk,uint64_t timestamp,const Row& row){
    std::string semanticText;
    for(int i=0;i<meta.columnCount;i++){
        if(meta.columns[i].isSemantic){
            semanticText+=meta.columns[i].name;
            semanticText+=": ";
            semanticText+=row.values[i];
            semanticText+=" ";
        }
    }
    if(semanticText.empty()) return false;
    const std::filesystem::path dir="data/embedding_queue";
    const std::filesystem::path tempPath=dir/"temp_tasks.queue";
    const std::filesystem::path lockPath=dir/"queue.lock";
    std::filesystem::create_directories(dir);
    if(!acquireQueueLock(lockPath,5000)){
        std::cerr<<"Timed out waiting for the global embedding queue lock.\n";
        return false;
    }
    bool success=false;
    do{
        std::fstream file(tempPath, std::ios::in|std::ios::out|std::ios::binary);
        if(!file){
            std::ofstream create(tempPath,std::ios::binary);
            if(!create){
                std::cerr<<"Failed to create global embedding queue.\n";
                break;
            }
            uint32_t count=0;
            writeBinary(create,count);
            create.close();
            file.open(tempPath,std::ios::in|std::ios::out|std::ios::binary);
        }
        if(!file){
            std::cerr<<"Failed to open global embedding queue.\n";
            break;
        }
        uint32_t count=0;
        file.seekg(0,std::ios::beg);
        readBinary(file,count);
        if(!file){
            std::cerr<<"Failed to read global embedding queue header.\n";
            break;
        }
        file.clear();
        file.seekp(0,std::ios::end);
        writeString(file,std::string(meta.name));
        writeString(file,pk);
        writeBinary(file,timestamp);
        writeString(file,semanticText);
        if(!file){
            std::cerr<<"Failed to append record to global embedding queue.\n";
            break;
        }
        ++count;
        file.clear();
        file.seekp(0,std::ios::beg);
        writeBinary(file,count);
        file.flush();
        success=file.good();
    }while(false);
    releaseQueueLock(lockPath);
    return success;
}

std::vector<float> strToEmbed(const std::string &str){
    const int PYTHON_TIMEOUT_MS = 120000;
    const std::filesystem::path dir="data/embedding_queue";
    const std::filesystem::path tempqueryFile=dir/"temp_query.queue";
    const std::filesystem::path queryFile=dir/"query.queue";
    const std::filesystem::path embedFile=dir/"query_embedding.vec";
    const std::filesystem::path doneFile=dir/"query_done.signal";
    std::filesystem::create_directories(dir);

    std::error_code ec;
    std::filesystem::remove(doneFile, ec);
    std::filesystem::remove(embedFile, ec);
    {
    std::ofstream file(tempqueryFile,std::ios::binary | std::ios::trunc);
    if(!file){
        std::cerr<<"Failed to create query queue file.\n";
        return {};
    }

    writeString(file, str);
    file.flush();
    }

    try {
        std::filesystem::rename(tempqueryFile, queryFile);
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Failed to publish query: " << e.what() << "\n";
        return {};
    }
    auto pythonStart = std::chrono::steady_clock::now();
    bool done = false;

    while(true){
        if(fs::exists(doneFile)){
            done = true;
            break;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - pythonStart).count();

        if(elapsed >= PYTHON_TIMEOUT_MS)
            break;

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    if(!done){
        std::cerr << "Timed out waiting for Python worker.\n";
        return {};
    }

    std::vector<float> embedding(VEC_DIM);
    std::ifstream embed(embedFile, std::ios::binary);
    if(!embed){
        std::cerr << "Failed to open " << embedFile << ".\n";
        return {};
    }

    embed.read(reinterpret_cast<char*>(embedding.data()), static_cast<std::streamsize>(sizeof(float) * embedding.size()));

    if(!embed){
        std::cerr << "Failed to read embeddings from " << embedFile << ".\n";
        return {};
    }

    embed.close();
    std::filesystem::remove(embedFile, ec);
    std::filesystem::remove(doneFile, ec);

    return embedding;
    
}