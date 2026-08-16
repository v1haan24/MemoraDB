#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct QueueRecord{
    std::string tableName;
    std::string pk;
    uint64_t timestamp = 0;
    std::string text;
};

bool acquireQueueLock(const fs::path& lockDir, int timeoutMs);
void releaseQueueLock(const fs::path& lockDir);
bool readQueueCount(const fs::path& path,uint32_t& count);
bool readQueueRecords(const fs::path& path, std::vector<QueueRecord>& records);