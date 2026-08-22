// ============================================================================
// test_live_pipeline_windows.cpp
//
// Windows-native version of the live end-to-end test. Same idea as
// test_live_pipeline.cpp (the POSIX/WSL/Linux version): this process plays
// "the main db process" and spawns the other two real MemoraDB processes
// as actual child processes, exactly as they'd run in production, then
// validates everything purely by reading the resulting files back off disk:
//
//   1. This process (main db)  -- creates the table (data.db), inserts rows
//                                  via Table::insert(), which internally
//                                  calls Table::writeQueue() and appends to
//                                  data/embedding_queue/temp_tasks.queue.
//                                  Nothing here is mocked.
//
//   2. embedMake.exe (child)   -- the real compiled src/vector/embedMake.cpp
//                                  binary, running its real infinite
//                                  watch-the-queue loop.
//
//   3. python (child)          -- the real src/vector/ml_worker.py, running
//                                  sentence-transformers / all-MiniLM-L6-v2,
//                                  computing real embeddings and writing
//                                  them back through the real queue files.
//
// Process spawning/watching/killing is delegated to winprocess.h/.cpp
// (Win32 CreateProcess / TerminateProcess) instead of POSIX fork/exec/kill.
// That split exists because <windows.h> typedefs INT / FLOAT / BOOL as
// Win32 types, which collide directly with this project's
// `enum DataType {INT, FLOAT, STRING, BOOL}` in common/metadata.h -- so
// <windows.h> and metadata.h can never appear in the same translation
// unit. winprocess.h exposes an opaque handle with no Win32 types in its
// signature, so this file never needs to see <windows.h> at all. Everything
// else here -- table creation, row inserts, polling data.db and the .vec
// file, correctness checks, the semantic-similarity check -- is identical
// logic to the Linux version.
//
// --------------------------------------------------------------------------
// Prerequisites
// --------------------------------------------------------------------------
//  - The vecTable.cpp fix (you mentioned you've already applied it).
//    Without it, batches with more than one record for the same table
//    silently drop every record after the first.
//  - A working MinGW-w64 g++ toolchain on Windows (e.g. via MSYS2 --
//    "mingw-w64-x86_64-gcc" package -- or WinLibs). MSVC would need small
//    adjustments (CreateProcessA/GetLastError etc. all exist in MSVC too,
//    but you'd compile winprocess.cpp and this file as separate /TP units).
//  - Python 3 installed and on PATH (python.exe), with:
//        pip install sentence-transformers
//    This needs network access to huggingface.co to download the model
//    weights the first time it runs.
//  - embedMake.exe already built (see build_and_run_windows.bat).
//
// --------------------------------------------------------------------------
// Usage
// --------------------------------------------------------------------------
//   test_live_pipeline_windows.exe <embedmake_exe> <python_worker_script> [worker_timeout_ms] [python_exe]
//
// Example (real MiniLM model):
//   test_live_pipeline_windows.exe embedMake.exe src\vector\ml_worker.py 180000 python
//
// Run this from the directory where you want data\ and logs\ created (your
// project root is the natural choice). See build_and_run_windows.bat for a
// one-command wrapper.
// ============================================================================

#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "winprocess.h"

#include "../src/catalog/catalog.h"
#include "../src/common/constants.h"
#include "../src/common/metadata.h"
#include "../src/vector/vecTable.h"
#include "../src/vector/vector_meta.h"

namespace fs = std::filesystem;
using steady_clock_t = std::chrono::steady_clock;

// ---------------------------------------------------------------------------
// Minimal test harness
// ---------------------------------------------------------------------------
static int g_checks = 0;
static int g_failures = 0;

static void check(bool cond, const std::string& description) {
    ++g_checks;
    std::cout << (cond ? "  [PASS] " : "  [FAIL] ") << description << "\n";
    if (!cond) ++g_failures;
}

static void section(const std::string& title) {
    std::cout << "\n== " << title << " ==\n";
}

// RAII guard: whatever happens below, every spawned process gets killed.
struct ProcessGuard {
    std::vector<ChildProcessHandle*> handles;
    ~ProcessGuard() {
        for (auto it = handles.rbegin(); it != handles.rend(); ++it) {
            terminateProcessWin(*it);
        }
    }
};

static bool waitForMarker(const fs::path& logPath, const std::string& marker, int timeoutMs) {
    auto start = steady_clock_t::now();
    while (true) {
        if (fs::exists(logPath)) {
            std::ifstream in(logPath);
            std::string content((std::istreambuf_iterator<char>(in)),
                                 std::istreambuf_iterator<char>());
            if (content.find(marker) != std::string::npos) return true;
        }
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           steady_clock_t::now() - start)
                           .count();
        if (elapsed >= timeoutMs) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

template <typename Pred>
static bool waitUntil(Pred predicate, int timeoutMs, int pollMs = 50) {
    auto start = steady_clock_t::now();
    while (true) {
        if (predicate()) return true;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           steady_clock_t::now() - start)
                           .count();
        if (elapsed >= timeoutMs) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(pollMs));
    }
}

// ---------------------------------------------------------------------------
static double cosineSimilarity(const float* a, const float* b, int dim) {
    double dot = 0.0, na = 0.0, nb = 0.0;
    for (int i = 0; i < dim; ++i) {
        dot += static_cast<double>(a[i]) * b[i];
        na += static_cast<double>(a[i]) * a[i];
        nb += static_cast<double>(b[i]) * b[i];
    }
    if (na <= 0.0 || nb <= 0.0) return 0.0;
    return dot / (std::sqrt(na) * std::sqrt(nb));
}

static TableMeta makeTableMeta(const std::string& name, std::vector<ColMeta> cols) {
    TableMeta meta{};
    std::strncpy(meta.name, name.c_str(), tns - 1);
    meta.name[tns - 1] = '\0';
    meta.columns = std::move(cols);
    return meta;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0]
                  << " <embedmake_exe> <python_worker_script> [worker_timeout_ms] [python_exe]\n";
        return 2;
    }
    const std::string embedMakeBin = argv[1];
    const std::string workerScript = argv[2];
    const int workerReadyTimeoutMs = argc > 3 ? std::stoi(argv[3]) : 180000;
    const std::string pythonExe = argc > 4 ? argv[4] : "python";
    const int pipelineTimeoutMs = 60000;

    std::cout << "===========================================================\n";
    std::cout << " MemoraDB LIVE End-to-End Test (Windows)\n";
    std::cout << " main db process  +  real embedMake.exe process  +  real\n";
    std::cout << " ml_worker.py process, talking over the actual queue files\n";
    std::cout << "===========================================================\n";
    std::cout << "embedMake binary : " << embedMakeBin << "\n";
    std::cout << "python worker    : " << workerScript << "\n";
    std::cout << "python exe       : " << pythonExe << "\n";

    if (fs::exists("data")) fs::remove_all("data");
    if (fs::exists("logs")) fs::remove_all("logs");

    ProcessGuard guard;

    section("Starting background processes");

    ChildProcessHandle* embedMakeProc = spawnProcessWin({embedMakeBin}, "logs/embedmake.log");
    guard.handles.push_back(embedMakeProc);
    bool embedMakeReady = embedMakeProc != nullptr &&
                           waitForMarker("logs/embedmake.log", "Watching:", 10000);
    check(embedMakeReady, "embedMake.exe started and is watching the queue directory");

    ChildProcessHandle* workerProc = spawnProcessWin({pythonExe, "-u", workerScript}, "logs/ml_worker.log");
    guard.handles.push_back(workerProc);
    std::cout << "  (waiting up to " << workerReadyTimeoutMs / 1000
              << "s for the ML worker to become ready -- this covers model "
                 "download+load on first run)\n";
    bool workerReady = workerProc != nullptr &&
                        waitForMarker("logs/ml_worker.log",
                                      "Global worker active and listening", workerReadyTimeoutMs);
    check(workerReady, "python ML worker process started and signaled readiness");

    if (!embedMakeReady || !workerReady) {
        std::cout << "\nOne or both background processes failed to start; see logs\\ for details.\n";
        std::cout << "\n=== " << (g_checks - g_failures) << " / " << g_checks << " checks passed ===\n";
        fs::remove_all("data");
        return 1;
    }

    section("Main db process: create table + insert rows (real writeQueue path)");
    {
    const std::string tableName = "documents";
    ColMeta idCol("id", INT, /*pk=*/true);
    ColMeta titleCol("title", STRING, /*pk=*/false, 64);
    titleCol.isSemantic = true;
    ColMeta contentCol("content", STRING, /*pk=*/false, 256);
    contentCol.isSemantic = true;
    TableMeta meta = makeTableMeta(tableName, {idCol, titleCol, contentCol});

    Catalog catalog;
    bool created = catalog.createTable(meta);
    check(created, "Catalog::createTable succeeds for '" + tableName + "'");
    check(fs::exists("data/" + tableName + "/data.db"), "data.db exists on disk");

    Table* table = catalog.getTable(tableName);
    check(table != nullptr, "table retrievable from catalog");
    if (!table) return 1;

    struct Doc { int id; std::string title; std::string content; };
    std::vector<Doc> docs = {
        {1, "Cats", "Cats are small domesticated carnivorous mammals valued as household pets."},
        {2, "Kittens", "Kittens are young cats that love to play, climb, and sleep most of the day."},
        {3, "Rocket Engines", "Rocket engines produce thrust by expelling high speed exhaust gas."},
    };

    std::vector<std::string> pks;
    std::vector<uint64_t> insertedTimestamps;

    for (const auto& d : docs) {
        Row row;
        row.values = {std::to_string(d.id), d.title, d.content};
        bool inserted = table->insert(row); // real writeQueue() path
        check(inserted, "row id=" + std::to_string(d.id) + " inserted into data.db and queued for embedding");

        std::string pk = std::to_string(d.id);
        Record rec = table->latest(pk);
        check(!rec.deleted && rec.row.values[1] == d.title && rec.row.values[2] == d.content,
              "row id=" + pk + " round-trips correctly out of data.db");

        pks.push_back(pk);
        insertedTimestamps.push_back(rec.timestamp);
    }

    check(fs::exists("data/embedding_queue"),
          "the shared embedding queue directory was created by writeQueue()");

    section("Waiting for the live pipeline to produce " + tableName + ".vec");

    const fs::path vecPath = fs::path("data") / tableName / (tableName + ".vec");
    bool vecFileAppeared = waitUntil([&]() { return fs::exists(vecPath); }, pipelineTimeoutMs);
    check(vecFileAppeared, tableName + ".vec was created by the live embedMake process");

    vecMeta vm;
    bool allRecordsLanded = waitUntil(
        [&]() {
            VectorMeta m = vm.readMetadata(tableName + ".vec");
            return m.payloadSize != 0 && m.recordCount == docs.size();
        },
        pipelineTimeoutMs);
    check(allRecordsLanded,
          "all " + std::to_string(docs.size()) + " embeddings landed in " + tableName +
              ".vec (recordCount matches)");

    section("Validating " + tableName + ".vec contents produced by the live worker");

    VectorMeta finalMeta = vm.readMetadata(tableName + ".vec");
    check(finalMeta.recordCount == docs.size(),
          "final on-disk recordCount == " + std::to_string(docs.size()));

    if (finalMeta.payloadSize != 0) {
        vecTable vt(finalMeta);

        std::vector<VecRecord> records;
        for (uint32_t i = 0; i < finalMeta.recordCount; ++i) records.push_back(vt.readRecord(i));

        auto findByPk = [&](const std::string& pk) -> const VecRecord* {
            for (const auto& r : records) if (r.pk == pk) return &r;
            return nullptr;
        };

        for (size_t i = 0; i < pks.size(); ++i) {
            const VecRecord* rec = findByPk(pks[i]);
            check(rec != nullptr, tableName + ".vec has a record for pk=" + pks[i]);
            if (!rec) continue;

            check(rec->timestamp == insertedTimestamps[i],
                  "pk=" + pks[i] + " timestamp in .vec matches the timestamp data.db recorded");

            bool nonZero = false, hasNaN = false;
            for (float v : rec->embedding) {
                if (v != 0.0f) nonZero = true;
                if (std::isnan(v)) hasNaN = true;
            }
            check(nonZero, "pk=" + pks[i] + " embedding is not all-zero (a real vector was written)");
            check(!hasNaN, "pk=" + pks[i] + " embedding contains no NaNs");
        }

        // With the real MiniLM model this should be a robust, reliable
        // signal.
        const VecRecord* cats = findByPk("1");
        const VecRecord* kittens = findByPk("2");
        const VecRecord* rockets = findByPk("3");
        if (cats && kittens && rockets) {
            double simCatsKittens = cosineSimilarity(cats->embedding, kittens->embedding, VEC_DIM);
            double simCatsRockets = cosineSimilarity(cats->embedding, rockets->embedding, VEC_DIM);
            double simKittensRockets = cosineSimilarity(kittens->embedding, rockets->embedding, VEC_DIM);

            std::cout << "  cos(cats, kittens)    = " << simCatsKittens << "\n";
            std::cout << "  cos(cats, rockets)    = " << simCatsRockets << "\n";
            std::cout << "  cos(kittens, rockets) = " << simKittensRockets << "\n";

            check(simCatsKittens > simCatsRockets,
                  "'Cats' is embedded closer to 'Kittens' than to 'Rocket Engines'");
            check(simCatsKittens > simKittensRockets,
                  "'Kittens' is embedded closer to 'Cats' than to 'Rocket Engines'");
        }
    }

    std::cout << "\n===========================================================\n";
    std::cout << " " << (g_checks - g_failures) << " / " << g_checks << " checks passed\n";
    std::cout << "===========================================================\n";
    if (g_failures > 0) {
        std::cout << "See logs\\embedmake.log and logs\\ml_worker.log for background-process output.\n";
    }

    }
    // Stop background processes before deleting their files.
    // Windows does not allow removing files that are still open.
    terminateProcessWin(workerProc);
    terminateProcessWin(embedMakeProc);

    guard.handles.clear();

    fs::remove_all("data");

    return g_failures == 0 ? 0 : 1;
}
