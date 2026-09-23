#pragma once
#include <memory>
#include <string>
#include <vector>
#include "../catalog/catalog.h"
#include "../engine/executor.h"
#include "../vector/minilm_embedder.h"

class Repl {
    Catalog catalog;
    Executor executor;
    std::unique_ptr<MiniLmEmbedder> embedder;
    std::vector<std::string> history;
    std::string pending;
    bool running = true;

    void showTokens(const std::string& sql);
    void dispatchMeta(const std::string& command);
    bool readStatement(std::string& statement, std::string& meta);
    void executeProgram(const std::string& input);
    void printResult(const ExecResult& result);

public:
    Repl();
    void run();
    int runCommandLine(int argc, char** argv);
};
