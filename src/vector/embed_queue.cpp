#include "embed_queue.h"

#include "../storage/table.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <utility>

bool acquireQueueLock(const fs::path& lockDir, int timeoutMs){
    const auto start = std::chrono::steady_clock::now();

    while(true){
        std::error_code ec;
        if(fs::create_directory(lockDir, ec))
            return true;

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

        if(elapsed >= timeoutMs)
            return false;

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void releaseQueueLock(const fs::path& lockDir){
    std::error_code ec;
    fs::remove(lockDir, ec);
}

bool readQueueCount(const fs::path& path, uint32_t& count){
    std::ifstream file(path, std::ios::binary);
    if(!file)
        return false;

    readBinary(file, count);
    return file.good();
}

bool readQueueRecords(const fs::path& path, std::vector<QueueRecord>& records){
    std::ifstream file(path, std::ios::binary);
    if(!file){
        std::cerr << "Failed to open " << path << " for reading.\n";
        return false;
    }

    uint32_t count = 0;
    readBinary(file, count);

    if(!file){
        std::cerr << "Failed to read queue header.\n";
        return false;
    }

    records.clear();
    records.reserve(count);

    for(uint32_t i = 0; i < count; ++i){
        QueueRecord record;

        readString(file, record.tableName);
        readString(file, record.pk);
        readBinary(file, record.timestamp);
        readString(file, record.text);

        if(!file){
            std::cerr<<"Queue record "<< i << " is incomplete or corrupt.\n";
            return false;
        }

        records.push_back(std::move(record));
    }

    return true;
}