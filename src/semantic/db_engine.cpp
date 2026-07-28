#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>

using namespace std;
using namespace std::chrono;

int main()
{
    int runs = 100;
    string queue_file = "benchmark.queue";

    string text = "natural language processing";

    // Clear any old queue file before starting
    ofstream clear_file(queue_file, ios::trunc);
    clear_file.close();

    cout << "\n=========================================\n";
    cout << "Starting Async C++ Benchmark (" << runs << " runs)\n";
    cout << "=========================================\n\n";

    // Start the C++ timer
    auto start = high_resolution_clock::now();

    // OPTIMIZATION 1: Open the file ONCE outside the loop
    ofstream q_file(queue_file, ios::app);
    if (q_file.is_open())
    {
        for (int i = 0; i < runs; i++)
        {
            q_file << text << "\n";
        }
        // Close the file after all 100 writes are done
        q_file.close();
    }

    // Stop the C++ timer
    auto end = high_resolution_clock::now();
    double elapsed = duration<double, milli>(end - start).count();

    cout << "----------- C++ Summary -----------\n";
    cout << "Items Queued       : " << runs << "\n";
    cout << "C++ Execution Time : " << elapsed << " ms\n";
    cout << "Status             : C++ thread is free. Python is processing in background.\n\n";

    return 0;
}
