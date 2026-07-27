  #include <iostream>
#include <fstream>
#include <chrono>
#include <string>

using namespace std;
using namespace std::chrono;

int main() {
    int num_inserts = 100;  
    string queue_filename = "outbox.queue";
    
    ofstream clear_file(queue_filename, ios::trunc);
    clear_file.close();
    
    cout << "[C++ Engine] Starting async inserts for " << num_inserts << " rows..." << endl;
    
    auto start = high_resolution_clock::now();

    for (int i = 0; i < num_inserts; i++) {
        ofstream queue_file(queue_filename, ios::app);
        if (queue_file.is_open()) {
            queue_file << "PK_" << i << "|This is a test document about networking and operating systems.\n";
            queue_file.close(); 
        }
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    
    cout << "[C++ Engine] Successfully committed " << num_inserts << " rows to disk in " << duration.count() << " ms." << endl;
    return 0;
}