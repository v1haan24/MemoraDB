#pragma once

#include <memory>
#include <string>
#include <vector>

class MiniLmEmbedder {
public:
    explicit MiniLmEmbedder(const std::string& modelDirectory);
    ~MiniLmEmbedder();
    MiniLmEmbedder(MiniLmEmbedder&&) noexcept;
    MiniLmEmbedder& operator=(MiniLmEmbedder&&) noexcept;
    MiniLmEmbedder(const MiniLmEmbedder&) = delete;
    MiniLmEmbedder& operator=(const MiniLmEmbedder&) = delete;

    std::vector<float> encode(const std::string& text) const;
    std::vector<float> encode(const std::vector<std::string>& texts) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
