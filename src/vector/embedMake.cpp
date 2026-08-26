#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>

#include "vecTable.h"
#include "vector_meta.h"
#include "../catalog/catalog.h"
#include "embed_queue.h"

namespace fs = std::filesystem;

const int BATCH_TARGET = 10;
const int MAX_WAIT_MS = 50;
const int RENAME_RETRIES = 20;
const int RENAME_RETRY_DELAY_MS = 5;
const int QUEUE_LOCK_TIMEOUT_MS = 5000;
const int PYTHON_TIMEOUT_MS = 120000;


int main(){
    const fs::path dir = "data/embedding_queue";
    const fs::path tempQueueFile = dir / "temp_tasks.queue";
    const fs::path queueFile = dir / "tasks.queue";
    const fs::path lockDir = dir / "queue.lock";
    const fs::path vecFile = dir / "embeddings.vec";
    const fs::path doneFile = dir / "done.signal";
    const fs::path tempVecFile = dir / "temp_embeddings.vec";

    fs::create_directories(dir);

    std::cout << "========================================" <<std::endl;
    std::cout << " MemoraDB Global Embedding C++ Worker" <<std::endl;
    std::cout << "========================================" <<std::endl;
    std::cout << "Watching: " << dir << std::endl;

    Catalog catalog;

    while(true){
        /*
            A tasks.queue may already exist if this worker restarted after
            the producer/consumer handoff. Process that recovered batch first.
        */
        fs::path activeQueueFile;

        if(fs::exists(queueFile)){
            activeQueueFile = queueFile;
        }
        else if(fs::exists(tempQueueFile)){
            uint32_t count = 0;
            if(!readQueueCount(tempQueueFile, count)){
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }

            if(count == 0){
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }

            auto start = std::chrono::steady_clock::now();

            while(count < BATCH_TARGET){
                std::this_thread::sleep_for(std::chrono::milliseconds(1));

                if(!readQueueCount(tempQueueFile, count))
                    break;

                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

                if(elapsed >= MAX_WAIT_MS)
                    break;
            }

            /*
                Lock out writeQueue() while taking the completed batch.
                This makes the temp-file -> queue-file handoff atomic with
                respect to producers, even though they are separate processes.
            */
            if(!acquireQueueLock(lockDir, QUEUE_LOCK_TIMEOUT_MS)){
                std::cerr << "Timed out waiting for global queue lock.\n";
                continue;
            }

            bool renamed = false;

            try{
                if(fs::exists(tempQueueFile)){
                    uint32_t lockedCount = 0;
                    if(readQueueCount(tempQueueFile, lockedCount) && lockedCount > 0){
                        for(int attempt = 0; attempt < RENAME_RETRIES; ++attempt){
                            try{
                                if(fs::exists(queueFile))
                                    fs::remove(queueFile);

                                fs::rename(tempQueueFile, queueFile);
                                renamed = true;
                                break;
                            }
                            catch(const fs::filesystem_error& e){
                                if(attempt == RENAME_RETRIES - 1)
                                    std::cerr << "Queue rename failed: " << e.what() << "\n";
                                else
                                    std::this_thread::sleep_for(std::chrono::milliseconds(RENAME_RETRY_DELAY_MS));
                            }
                        }
                    }
                }
            }
            catch(...){
                releaseQueueLock(lockDir);
                throw;
            }

            releaseQueueLock(lockDir);

            if(!renamed)
                continue;

            activeQueueFile = queueFile;
        }
        else{
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        std::vector<QueueRecord> records;
        if(!readQueueRecords(activeQueueFile, records)){
            std::cerr << "Failed to read the batch. Removing corrupted queue file.\n";
            fs::remove(activeQueueFile);
            continue;
        }

        const size_t batchSize = records.size();
        if(batchSize == 0){
            fs::remove(queueFile);
            continue;
        }

        std::cout << "\nReceived " << batchSize << " record(s) from global queue:" << std::endl;
        for(const auto& record : records){
            std::cout << "  " << record.tableName << " / " << record.pk << std::endl;
        }

        /* Remove stale result artifacts before asking Python for this batch. */
        std::error_code ec;
        fs::remove(doneFile, ec);
        fs::remove(vecFile, ec);
        fs::remove(tempVecFile, ec);

        std::cout << "Waiting for embeddings..." << std::endl;

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
            std::cerr << "The queue file is retained at " << queueFile << " so the batch is not silently discarded.\n";
            continue;
        }

        std::vector<float> embeddings(batchSize * VEC_DIM);
        std::ifstream vecInput(vecFile, std::ios::binary);

        if(!vecInput){
            std::cerr << "Failed to open " << vecFile << ".\n";
            continue;
        }

        vecInput.read(reinterpret_cast<char*>(embeddings.data()), static_cast<std::streamsize>(sizeof(float) * embeddings.size()));

        if(!vecInput){
            std::cerr << "Failed to read all embeddings from " << vecFile << ".\n";
            continue;
        }

        vecInput.close();

        /*
            Group records by table so one VectorMeta/vecTable object is used
            for each table in the batch.
        */
        std::unordered_map<std::string, std::vector<int>> groups;
        for(int i = 0; i < records.size(); ++i)
            groups[records[i].tableName].push_back(i);

        
        vecMeta vm;

        int inserted = 0;

        for(const auto& [tableName, indices] : groups){

            Table* table = catalog.getTable(tableName);

            if(!table){
                std::cout << "Table '" << tableName << "' not found in cached catalog. Refreshing...\n";
                if(catalog.loadTable(tableName))
                    table = catalog.getTable(tableName);
            }

            if(!table){
                std::cerr << "Table '" << tableName << "' no longer exists in the catalog.\n";
                continue;
            }

            VectorMeta meta = vm.readMetadata(tableName + ".vec");

            if(meta.payloadSize == 0){
                std::cout << "Creating vector table for '" << tableName << "'..." << std::endl;

                if(!vm.createVecTable(meta, table->getMeta())){
                    std::cerr << "Failed to create vector table for '"<< tableName << "'.\n";
                    continue;
                }
            }

            vecTable vt(meta);

            for(int index : indices){
                const QueueRecord& record = records[index];

                if(!vt.insert(record.pk,record.timestamp,embeddings.data() + index * VEC_DIM)){
                    std::cerr << "Failed to insert embedding for '"<< record.tableName << " / "<< record.pk << "'.\n";
                    continue;
                }

                ++inserted;
            }
        }

        fs::remove(queueFile);
        fs::remove(vecFile);
        fs::remove(doneFile);
        fs::remove(tempVecFile);

        std::cout << "Inserted " << inserted << " / " << batchSize << " embedding(s) into the appropriate vector tables." << std::endl;
    }

    return 0;
}
