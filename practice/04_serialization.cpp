#include<iostream>
#include<fstream>
#include<cstring>
#include<stdio.h>
#include <iomanip> // Used for formatting
using namespace std;

class employee
{
    public:

    int id = 0;
    int salary = 0;
    bool onsite = false;
    string name;
    string addr;

    employee() {}

    employee(int i, int s, bool os, string n, string a)
    {
        id = i;
        salary = s;
        onsite = os;
        name =  n;
        addr = a;
    }

    void display() 
    { 
        //Left-aligns the text inside its assigned column width
        //setw(X) Reserves exactly X characters of space for the very next item being printed. If the data is shorter than X, it pads it with spaces.
        cout<< left           
            << setw(10) << id 
            << setw(15) << name 
            << setw(15) << addr 
            << setw(12) << salary 
            << setw(10) << (onsite ? "Yes" : "No") 
            << "\n";
    }
};

void writeEmployee(ofstream& file, const employee& e)
{
    file.write(reinterpret_cast<const char*>(&e.id), sizeof(e.id));
    file.write(reinterpret_cast<const char*>(&e.salary),sizeof(e.salary));
    file.write(reinterpret_cast<const char*>(&e.onsite),sizeof(e.onsite));

    int len = e.name.size();
    file.write(reinterpret_cast<const char*>(&len),sizeof(len));
    file.write(e.name.c_str(),len);

    len = e.addr.size();
    file.write(reinterpret_cast<const char*>(&len),sizeof(len));
    file.write(e.addr.c_str(),len);

}

bool readEmployee(ifstream& file, employee& e)
{
    // Check every single read statement sequentially. If any fail, return false immediately.
    if(!file.read(reinterpret_cast<char*>(&e.id), sizeof(e.id))) return false;

    if(!file.read(reinterpret_cast<char*>(&e.salary),sizeof(e.salary))) return false;
    if(!file.read(reinterpret_cast<char*>(&e.onsite),sizeof(e.onsite))) return false;

    int len;
    if(!file.read(reinterpret_cast<char*>(&len),sizeof(len))) return false;
    e.name.resize(len);
    if(!file.read(&e.name[0],len)) return false;

    if(!file.read(reinterpret_cast<char*>(&len),sizeof(len))) return false;
    e.addr.resize(len);
    if(!file.read(&e.addr[0],len)) return false;

    return true;
}

void readData() 
{
    ifstream file("memora.db", ios::binary);
    if (!file) {
        cerr << "Error: Could not open file for reading.\n";
        return;
    }

    cout << "----- Employee Details -----\n";
    cout << left << setw(10) << "ID" << setw(15) << "Name" << setw(15) << "Address" << setw(12) << "Salary" << setw(10) << "Onsite" << "\n";
    employee e;
    while (readEmployee(file, e)) {
        e.display();
    }
}

int main() {
    
    ofstream file("memora.db", ios::binary);
    if (!file) {
        cerr << "Error: Could not open file for writing.\n";
        return 1;
    }

    employee e1(101, 65000, true, "Naman", "Mumbai");
    employee e2(102, 50000, false, "Raj", "Pune");
    writeEmployee(file, e1);
    writeEmployee(file, e2);
    file.close();

    readData();
    return 0;
}