#include <iostream>
#include <fstream>
#include <cstring>
using namespace std;

// I'm setting up a simple Employee struct here just to test how C++ handles saving our data in pure binary.
struct Employee {
    int emp_id;
    char name[30];
    float salary;
    bool is_active;
};

int main() {
    // Testing with some standard employee data
    Employee e1 = {101, "Rahul", 55000.50, true};
    Employee e2 = {102, "Aman", 62000.00, false};

    // I am opening the file in binary mode, but also 'app' (append) mode. 
    // This makes sure we don't accidentally overwrite the old employees every time we run the code.
    ofstream outFile("employee_test.dat", ios::binary | ios::app);
    outFile.write(reinterpret_cast<char*>(&e1), sizeof(Employee));
    outFile.write(reinterpret_cast<char*>(&e2), sizeof(Employee));
    outFile.close();

    // Now I'm reading it back into a temporary struct to prove the binary wasn't corrupted.
    ifstream inFile("employee_test.dat", ios::binary);
    Employee temp;
    
    cout << "Reading employee records from the binary test file:\n";
    while (inFile.read(reinterpret_cast<char*>(&temp), sizeof(Employee))) {
        cout << "ID: " << temp.emp_id << " | Name: " << temp.name 
             << " | Salary: $" << temp.salary << " | Active: " << temp.is_active << "\n";
    }
    inFile.close();

    return 0;
}