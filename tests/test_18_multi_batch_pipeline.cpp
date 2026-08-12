#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <cstdlib>

#include "../src/catalog/catalog.h"

using namespace std;
namespace fs = std::filesystem;

static bool fileExists(const string& path)
{
    return fs::exists(path);
}

static bool waitForQueueIdle(const string& queueDir, int timeoutMs)
{
    auto start = chrono::steady_clock::now();

    while (true)
    {
        bool tempTasks = fileExists(queueDir + "/temp_tasks.queue");
        bool tempDet   = fileExists(queueDir + "/temp_det.queue");
        bool tasks     = fileExists(queueDir + "/tasks.queue");
        bool det       = fileExists(queueDir + "/det.queue");

        if (!tempTasks && !tempDet && !tasks && !det)
            return true;

        auto elapsed =
            chrono::duration_cast<chrono::milliseconds>(
                chrono::steady_clock::now() - start
            ).count();

        if (elapsed >= timeoutMs)
            return false;

        this_thread::sleep_for(chrono::milliseconds(50));
    }
}

static void writeBatch(Table* table, int first, int count)
{
    for (int i = first; i < first + count; ++i)
    {
        string pk = "mb-" + to_string(i);

        string text =
            "multi batch semantic document number " +
            to_string(i);

        uint64_t timestamp = 200000ULL + static_cast<uint64_t>(i);

        Row row = {
            {pk, text, "multi-batch"}
        };

        assert(table->writeQueue(pk, timestamp, row));
    }
}

int main()
{
    cout << "\n======= Multi-Batch Pipeline Test =======\n\n";

    fs::remove_all("data/MultiBatchDocs");

    /*
        Create the test table.
        This matches the semantic table used by the pipeline.
    */
    Catalog catalog;

    TableMeta meta;
    //meta.name = "MultiBatchDocs";
    string nam = "MultiBatchDocs";
    nam.copy(meta.name, nam.length(), 0);
    meta.name[nam.length()] = '\0';

    meta.columns.push_back(
        ColMeta("docId", STRING, true, 16)
    );

    ColMeta text("text", STRING, false, 100);
    text.isSemantic = true;

    meta.columns.push_back(text);

    meta.columns.push_back(
        ColMeta("category", STRING, false, 20)
    );

    assert(catalog.createTable(meta));

    Table* table = catalog.getTable("MultiBatchDocs");
    assert(table != nullptr);

    const string queueDir =
        "data/MultiBatchDocs/queue";

    const string vecFile =
        "data/MultiBatchDocs/MultiBatchDocs.vec";

    /*
        ---------------------------------------------------------
        Batch 1: 10 records
        ---------------------------------------------------------
    */

    cout << "[1] Writing batch 1: records 0-9...\n";

    writeBatch(table, 0, 10);

    /*
        Start embedMake and the real Python worker.

        start command is executed through PowerShell so that
        Windows headers are not required by this C++ test.
    */

    cout << "[2] Starting embedMake.exe...\n";

    system(
        "start \"MemoraDB embedMake\" "
        "cmd /c \"embedMake.exe MultiBatchDocs\""
    );

    this_thread::sleep_for(
        chrono::milliseconds(500)
    );

    cout << "[3] Starting real ml_worker.py...\n";

    system(
        "start \"MemoraDB Python Worker\" "
        "cmd /c \"python src/semantic/ml_worker.py MultiBatchDocs\""
    );

    cout << "[4] Waiting for batch 1 to be consumed...\n";

    if (!waitForQueueIdle(queueDir, 120000))
    {
        cerr << "ERROR: Timed out waiting for batch 1.\n";
        return 1;
    }

    cout << "[5] Batch 1 consumed successfully.\n";

    /*
        ---------------------------------------------------------
        Batch 2: 10 records
        ---------------------------------------------------------
    */

    cout << "[6] Writing batch 2: records 10-19...\n";

    writeBatch(table, 10, 10);

    cout << "[7] Waiting for batch 2 to be consumed...\n";

    if (!waitForQueueIdle(queueDir, 120000))
    {
        cerr << "ERROR: Timed out waiting for batch 2.\n";
        return 1;
    }

    cout << "[8] Batch 2 consumed successfully.\n";

    /*
        ---------------------------------------------------------
        Batch 3: 5 records
        ---------------------------------------------------------
    */

    cout << "[9] Writing batch 3: records 20-24...\n";

    writeBatch(table, 20, 5);

    cout << "[10] Waiting for batch 3 to be consumed...\n";

    if (!waitForQueueIdle(queueDir, 120000))
    {
        cerr << "ERROR: Timed out waiting for batch 3.\n";
        return 1;
    }

    cout << "[11] Batch 3 consumed successfully.\n";

    /*
        Give embedMake a short amount of time to finish the
        final vector-table insertion.
    */

    this_thread::sleep_for(
        chrono::milliseconds(1000)
    );

    if (!fileExists(vecFile))
    {
        cerr << "ERROR: Final vector file was not created:\n"
             << vecFile << "\n";
        return 1;
    }

    cout << "[12] Final vector table exists:\n"
         << vecFile << "\n";

    cout << "\n=========================================\n";
    cout << "Multi-batch pipeline test passed.\n";
    cout << "Processed batches: 10 + 10 + 5 = 25 records.\n";
    cout << "=========================================\n";

    cout << "\nNOTE:\n";
    cout << "Two separate command windows were started for\n";
    cout << "embedMake.exe and ml_worker.py.\n";
    cout << "Close them manually after confirming the test output.\n";

    return 0;
}
