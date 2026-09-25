#pragma once
#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>

class TempDirectory {
private:
    std::filesystem::path originalPath;
    std::filesystem::path tempPath;

public:
    TempDirectory() {
        static std::atomic<uint64_t> counter{0};
        originalPath = std::filesystem::current_path();
        const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        tempPath = std::filesystem::temp_directory_path() / ("memora_test_" + std::to_string(now) + "_" + std::to_string(counter.fetch_add(1)));
        std::filesystem::create_directories(tempPath);
        std::filesystem::current_path(tempPath);
    }

    ~TempDirectory() {
        std::filesystem::current_path(originalPath);
        std::error_code ec;
        std::filesystem::remove_all(tempPath, ec);
    }

    const std::filesystem::path& path() const {
        return tempPath;
    }
};
