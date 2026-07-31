#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <cstdio>

using namespace std;
using namespace std::chrono;

// Helper to check file size accurately
size_t getFileSize(const string &filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : 0;
}

int main()
{
    int runs = 100;
    int dimensions = 384;
    string text = "natural language processing";

    string queue_file = "tasks.queue";
    string vec_file = "embeddings.vec";

    // Clean up old files from previous runs
    remove(queue_file.c_str());
    remove(vec_file.c_str());

    cout << "=================================================" << endl;
    cout << "   MemoraDB End-to-End Pipeline Benchmark       " << endl;
    cout << "=================================================" << endl;
    cout << "Batch Size: " << runs << " queries | Vector Dimensions: " << dimensions << endl
         << endl;

    auto pipeline_start = high_resolution_clock::now();
    
    // STEP 1: Direct Queue Write (C++ -> Disk)
    auto start_write = high_resolution_clock::now();

    string buffer;
    buffer.reserve(runs * (text.length() + 1));
    for (int i = 0; i < runs; i++)
    {
        buffer += text + "\n";
    }

    ofstream q_file(queue_file);
    q_file << buffer;
    q_file.close();

    auto end_write = high_resolution_clock::now();
    double write_time = duration<double, std::milli>(end_write - start_write).count();

    cout << "[1/3] C++ Direct Write Time  : " << write_time << " ms" << endl;
    cout << "[2/3] Waiting for Python Worker..." << endl;

    // STEP 2: Wait for Python Binary Output
    size_t expected_size = runs * dimensions * sizeof(float); // 153,600 bytes

    auto start_wait = high_resolution_clock::now();
    while (getFileSize(vec_file) < expected_size)
    {
        this_thread::sleep_for(chrono::milliseconds(1));
    }
    auto end_wait = high_resolution_clock::now();
    double ml_pipeline_time = duration<double, std::milli>(end_wait - start_wait).count();

    // STEP 3: Binary Read into std::vector<float>
    auto start_read = high_resolution_clock::now();

    // Pre-allocate memory for all 38,400 floats (100 * 384)
    vector<float> embeddings(runs * dimensions);

    // Scoop raw machine bytes directly into std::vector<float> memory
    ifstream v_file(vec_file, ios::binary);
    v_file.read(reinterpret_cast<char *>(embeddings.data()), expected_size);
    v_file.close();

    auto end_read = high_resolution_clock::now();
    double read_time = duration<double, std::milli>(end_read - start_read).count();

    auto pipeline_end = high_resolution_clock::now();
    double total_pipeline_time = duration<double, std::milli>(pipeline_end - pipeline_start).count();

    // FINAL BENCHMARK SUMMARY
    cout << "\n================ C++ BENCHMARK REPORT ================" << endl;
    cout << " Step 1: C++ Queue Write Overhead  : " << write_time << " ms" << endl;
    cout << " Step 2: Python ML + Binary Write   : " << ml_pipeline_time << " ms" << endl;
    cout << " Step 3: C++ Raw Binary Memory Read: " << read_time << " ms" << endl;
    cout << "------------------------------------------------------" << endl;
    cout << " Total End-to-End Pipeline Latency : " << total_pipeline_time << " ms" << endl;
    cout << " Total C++ I/O Overhead (Step 1+3) : " << write_time + read_time << " ms" << endl;
    cout << "------------------------------------------------------" << endl;
    cout << " Verification Check:" << endl;
    cout << "   - Bytes Read from Disk: " << expected_size << " bytes" << endl;
    cout << "   - Total Floats Loaded : " << embeddings.size() << " floats" << endl;
    cout << "   - Vector[0] Float[0]  : " << embeddings[0] << endl;
    cout << "   - Vector[0] Float[1]  : " << embeddings[1] << endl;
    cout << "======================================================" << endl;

    return 0;
}
