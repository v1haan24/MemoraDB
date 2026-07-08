#include <iostream>
#include <fstream>
#include <cstring>
using namespace std;

struct Employee {
    int emp_id;
    char name[30];
    float salary;
    bool is_deleted; 
};

void addEmployee(int id, string n, float sal) {
    Employee e;
    e.emp_id = id;
    strcpy(e.name, n.c_str());
    e.salary = sal;
    e.is_deleted = false;

    ofstream file("employee_test.dat", ios::binary | ios::app);
    file.write(reinterpret_cast<char*>(&e), sizeof(Employee));
    file.close();
}

void showEmployees() {
    ifstream file("employee_test.dat", ios::binary);
    Employee e;
    
    while (file.read(reinterpret_cast<char*>(&e), sizeof(Employee))) {
        // skip deleted records
        if (e.is_deleted == false) {
            cout << "ID: " << e.emp_id << " | Name: " << e.name << " | Salary: " << e.salary << "\n";
        }
    }
    file.close();
}

void deleteEmployee(int id) {
    // open in both read and write mode
    fstream file("employee_test.dat", ios::binary | ios::in | ios::out);
    Employee e;
    
    while (file.read(reinterpret_cast<char*>(&e), sizeof(Employee))) {
        if (e.emp_id == id && e.is_deleted == false) {
            e.is_deleted = true; // soft delete
            
            // step back and overwrite
            file.seekp(file.tellg() - sizeof(Employee), ios::beg);
            file.write(reinterpret_cast<char*>(&e), sizeof(Employee));
            
            cout << "Deleted employee " << id << "\n";
            break;
        }
    }
    file.close();
}

int main() {
    addEmployee(101, "Rahul", 55000);
    addEmployee(102, "Aman", 62000);
    addEmployee(103, "Priya", 71000);
    
    cout << "--- Before Delete ---\n";
    showEmployees();

    deleteEmployee(102);

    cout << "\n--- After Delete ---\n";
    showEmployees(); 

    return 0;
}