#include "minilm_embedder.h"

#include <onnxruntime_cxx_api.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <unordered_map>

namespace {
constexpr int64_t kMaxTokens = 256;
constexpr int64_t kDimensions = 384;

bool isWhitespace(unsigned char c) { return c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\f' || c == '\v'; }
bool isPunctuation(unsigned char c) {
    return (c >= '!' && c <= '/') || (c >= ':' && c <= '@') ||
           (c >= '[' && c <= '`') || (c >= '{' && c <= '~');
}

std::vector<std::string> basicTokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::string word;
    auto flush = [&] { if (!word.empty()) { tokens.push_back(word); word.clear(); } };
    for (unsigned char c : text) {
        if (isWhitespace(c)) { flush(); continue; }
        if (isPunctuation(c)) { flush(); tokens.emplace_back(1, static_cast<char>(c)); continue; }
        // all-MiniLM-L6-v2 is uncased. ASCII lowercasing preserves UTF-8 bytes.
        word.push_back(static_cast<char>(c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c));
    }
    flush();
    return tokens;
}
}

struct MiniLmEmbedder::Impl {
    Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "memora"};
    Ort::SessionOptions options;
    Ort::Session session{nullptr};
    std::unordered_map<std::string, int64_t> vocab;
    int64_t cls = 101, sep = 102, unk = 100, pad = 0;

    explicit Impl(const std::string& directory) {
        std::ifstream vocabFile(directory + "/vocab.txt");
        if (!vocabFile) throw std::runtime_error("Cannot open tokenizer vocabulary: " + directory + "/vocab.txt");
        std::string piece;
        for (int64_t id = 0; std::getline(vocabFile, piece); ++id) vocab.emplace(piece, id);
        auto findId = [&](const char* token, int64_t fallback) { auto it = vocab.find(token); return it == vocab.end() ? fallback : it->second; };
        cls = findId("[CLS]", cls); sep = findId("[SEP]", sep); unk = findId("[UNK]", unk); pad = findId("[PAD]", pad);
        options.SetIntraOpNumThreads(1);
        options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
#ifdef _WIN32
        std::wstring modelPath(directory.begin(), directory.end());
        modelPath += L"/model.onnx";
        session = Ort::Session(env, modelPath.c_str(), options);
#else
        session = Ort::Session(env, (directory + "/model.onnx").c_str(), options);
#endif
    }

    std::vector<int64_t> wordPieces(const std::string& token) const {
        std::vector<int64_t> ids;
        if (token.empty()) return ids;
        size_t start = 0;
        while (start < token.size()) {
            size_t end = token.size();
            bool found = false;
            while (end > start) {
                std::string piece = (start == 0 ? "" : "##") + token.substr(start, end - start);
                const auto it = vocab.find(piece);
                if (it != vocab.end()) { ids.push_back(it->second); start = end; found = true; break; }
                --end;
            }
            if (!found) return {unk};
        }
        return ids;
    }
};

MiniLmEmbedder::MiniLmEmbedder(const std::string& modelDirectory) : impl(std::make_unique<Impl>(modelDirectory)) {}
MiniLmEmbedder::~MiniLmEmbedder() = default;
MiniLmEmbedder::MiniLmEmbedder(MiniLmEmbedder&&) noexcept = default;
MiniLmEmbedder& MiniLmEmbedder::operator=(MiniLmEmbedder&&) noexcept = default;

std::vector<float> MiniLmEmbedder::encode(const std::string& text) const { return encode(std::vector<std::string>{text}); }

std::vector<float> MiniLmEmbedder::encode(const std::vector<std::string>& texts) const {
    if (texts.empty()) return {};
    const size_t batch = texts.size();
    std::vector<std::vector<int64_t>> tokenRows(batch);
    int64_t sequenceLength = 2; // [CLS] and [SEP]
    for (size_t row = 0; row < batch; ++row) {
        for (const auto& token : basicTokenize(texts[row])) {
            for (const auto id : impl->wordPieces(token)) {
                if (tokenRows[row].size() >= static_cast<size_t>(kMaxTokens - 2)) break;
                tokenRows[row].push_back(id);
            }
            if (tokenRows[row].size() >= static_cast<size_t>(kMaxTokens - 2)) break;
        }
        sequenceLength = std::max(sequenceLength, static_cast<int64_t>(tokenRows[row].size() + 2));
    }
    std::vector<int64_t> ids(batch * sequenceLength, impl->pad), masks(batch * sequenceLength, 0), types(batch * sequenceLength, 0);
    for (size_t row = 0; row < batch; ++row) {
        size_t position = 0;
        ids[row * sequenceLength + position] = impl->cls; masks[row * sequenceLength + position++] = 1;
        for (const auto id : tokenRows[row]) {
            ids[row * sequenceLength + position] = id; masks[row * sequenceLength + position++] = 1;
        }
        ids[row * sequenceLength + position] = impl->sep; masks[row * sequenceLength + position] = 1;
    }
    const std::array<int64_t, 2> shape{static_cast<int64_t>(batch), sequenceLength};
    auto memory = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::array<Ort::Value, 3> inputs{
        Ort::Value::CreateTensor<int64_t>(memory, ids.data(), ids.size(), shape.data(), shape.size()),
        Ort::Value::CreateTensor<int64_t>(memory, masks.data(), masks.size(), shape.data(), shape.size()),
        Ort::Value::CreateTensor<int64_t>(memory, types.data(), types.size(), shape.data(), shape.size())};
    const char* names[] = {"input_ids", "attention_mask", "token_type_ids"};
    const char* outputNames[] = {"last_hidden_state"};
    auto output = impl->session.Run(Ort::RunOptions{nullptr}, names, inputs.data(), inputs.size(), outputNames, 1);
    const float* hidden = output.front().GetTensorData<float>();
    std::vector<float> result(batch * kDimensions, 0.0F);
    for (size_t row = 0; row < batch; ++row) {
        float denominator = 0.0F;
        for (int64_t token = 0; token < sequenceLength; ++token) {
            if (!masks[row * sequenceLength + token]) continue;
            denominator += 1.0F;
            const float* source = hidden + (row * sequenceLength + token) * kDimensions;
            float* target = result.data() + row * kDimensions;
            for (int64_t dim = 0; dim < kDimensions; ++dim) target[dim] += source[dim];
        }
        float norm = 0.0F;
        float* target = result.data() + row * kDimensions;
        for (int64_t dim = 0; dim < kDimensions; ++dim) { target[dim] /= denominator; norm += target[dim] * target[dim]; }
        norm = std::sqrt(norm);
        if (norm > std::numeric_limits<float>::epsilon()) for (int64_t dim = 0; dim < kDimensions; ++dim) target[dim] /= norm;
    }
    return result;
}