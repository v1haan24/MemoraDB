#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <fstream>
#include <vector>
#include "vecTable.h"
#include "vector_meta.h"
#include "../catalog/catalog.h"

namespace fs = std::filesystem;

const int BATCH_TARGET = 10;   
const int MAX_WAIT_MS = 50;    
const int RENAME_RETRIES = 20; 
const int RENAME_RETRY_DELAY_MS = 5;

int main(int argc, char* argv[]){
    if(argc < 2){
        std::cerr<<"Usage: "<<argv[0]<<" <table_name>\n";
        return 1;
    }
    std::string tableName = argv[1];

    Catalog cat;
    Table* table = cat.getTable(tableName);
    if(!table){
        std::cerr<<"Table '"<<tableName<<"' not found in the catalog. Create it first.\n";
        return 1;
    }
    TableMeta tm = table->getMeta();

    vecMeta vm;
    VectorMeta meta = vm.readMetadata(tableName+".vec", tm);
    if(meta.payloadSize == 0){
        std::cout<<"No existing vector table for '"<<tableName<<"', creating one...\n";
        if(!vm.createVecTable(meta, tm)){
            std::cerr<<"Failed to create vector table for '"<<tableName<<"'.\n";
            return 1;
        }
    }
    vecTable vt(meta);

    std::string dir = "data/"+tableName+"/queue";
    fs::create_directories(dir);
    const std::string TEMP_QUEUE_FILE = dir+"/temp_tasks.queue";
    const std::string QUEUE_FILE = dir+"/tasks.queue";
    const std::string TEMP_DET_FILE = dir+"/temp_det.queue";
    const std::string DET_FILE = dir+"/det.queue";
    const std::string VEC_FILE = dir+"/embeddings.vec";
    const std::string DONE_FILE = dir+"/done.signal";

    std::cout<<"Embedding worker started for table '"<<tableName<<"'.\n";

    while(true){
        if(!fs::exists(TEMP_QUEUE_FILE) || fs::is_empty(TEMP_QUEUE_FILE)){
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        if(!fs::exists(TEMP_DET_FILE)){
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        int count = 0;
        {
            std::ifstream detHeader(TEMP_DET_FILE, std::ios::binary);
            if(!detHeader){ std::cerr<<"Failed to open "<<TEMP_DET_FILE<<" for reading.\n"; return 1; }
            readBinary(detHeader, count);
        }

        if (count >= 1)
        {
            auto start = std::chrono::steady_clock::now();

            while (count < BATCH_TARGET)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));

                auto now = std::chrono::steady_clock::now();

                auto elapsed =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - start
                    ).count();

                if (elapsed >= MAX_WAIT_MS)
                    break;
                
                {
                    std::ifstream detHeader(TEMP_DET_FILE, std::ios::binary);
                    if(!detHeader){ std::cerr<<"Failed to open "<<TEMP_DET_FILE<<" for reading.\n"; return 1; }
                    readBinary(detHeader, count);
                }
            }
        }

        bool renamed = false;
        for(int attempt = 0; attempt < RENAME_RETRIES; attempt++){
            try{
                fs::rename(TEMP_QUEUE_FILE, QUEUE_FILE);
                fs::rename(TEMP_DET_FILE, DET_FILE);
                renamed = true;
                break;
            } catch(const fs::filesystem_error& e){
                std::cerr<<"Rename attempt "<<(attempt+1)<<" failed: "<<e.what()<<", retrying...\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(RENAME_RETRY_DELAY_MS));
            }
        }
        if(!renamed){
            std::cerr<<"Giving up renaming queue files for '"<<tableName<<"' after "<<RENAME_RETRIES<<" attempts.\n";
            continue;
        }
        
        {
            std::ifstream detHeader(DET_FILE, std::ios::binary);
            if(!detHeader){ std::cerr<<"Failed to open "<<DET_FILE<<" for reading.\n"; return 1; }
            readBinary(detHeader, count);
        }

        if(count <= 0){
            fs::remove(QUEUE_FILE);
            fs::remove(DET_FILE);
            continue;
        }

        std::cout<<"Waiting for embeddings for "<<count<<" record(s)...\n";
        while(!fs::exists(DONE_FILE)){
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        std::vector<float> embeddings((size_t)count * VEC_DIM);
        std::ifstream vecFile(VEC_FILE, std::ios::binary);
        if(!vecFile){ std::cerr<<"Failed to open "<<VEC_FILE<<" for reading.\n"; return 1; }
        vecFile.read(reinterpret_cast<char*>(embeddings.data()), sizeof(float)*embeddings.size());
        if(!vecFile){ std::cerr<<"Failed to read embeddings from "<<VEC_FILE<<".\n"; return 1; }
        vecFile.close();
        fs::remove(DONE_FILE);

        std::ifstream detFile(DET_FILE, std::ios::binary);
        if(!detFile){ std::cerr<<"Failed to open "<<DET_FILE<<" for reading.\n"; return 1; }
        int detCount = 0;
        readBinary(detFile, detCount);
        int n = std::min(count, detCount);
        for(int i = 0; i < n; i++){
            std::string pk;
            uint64_t timestamp;
            readString(detFile, pk);      
            readBinary(detFile, timestamp);
            if(!vt.insert(pk, timestamp, embeddings.data() + (size_t)i*VEC_DIM)){
                std::cerr<<"Failed to insert embedding for pk '"<<pk<<"'.\n";
            }
        }
        detFile.close();

        fs::remove(QUEUE_FILE);
        fs::remove(DET_FILE);
        fs::remove(VEC_FILE);

        std::cout<<"Inserted "<<n<<" embedding(s) into "<<tableName<<".vec\n";
    }

    return 0;
}