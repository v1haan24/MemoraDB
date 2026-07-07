#include<iostream>
#include<fstream>
#include<cstring>
#include<ctime>
using namespace std;

class Student
{
    public:

    int id;
    int ver;
    char name[30]; //char array is used instead of string as string itself is an object and we cannot read an object into and object. 
    char addr[50];  //Object should be read in some primitive data type(int char bool etc.)
    bool flag; // 1 or true for INSERT/UPDATE     0 or false for DELETE
    char timestamp[30]; //Stores the date and time of a particular transaction

    Student() {}

    Student(int Id, bool fl, string nam, string add)
    {
        id = Id;
        flag = fl;
        strcpy(name, nam.c_str()); // This is done because we cannot copy a sting directly into a char array
        strcpy(addr, add.c_str());
        ver = 1;
        setTimestamp();
    }

    // Helper method to grab the current system time and format it clean
    void setTimestamp()
    {
        time_t now = time(nullptr);
        tm* localTime = localtime(&now);
        
        // Formats time safely into a string: "YYYY-MM-DD HH:MM:SS"
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localTime);
    }

    void info()
    {
        cout<<"ID: "<<id<<"\tVersion: "<<ver<<"\tTime: "<<timestamp<<"\tFlag:"<<flag<<"\tName: "<<name<<"\tAddress: "<<addr<<endl;
    }

};

void insert(Student s)
{
    ofstream f("details.db", ios::binary | ios::app); //Opens file in binary write mode and append 
    f.write(reinterpret_cast<char*> (&s), sizeof(s)); // Writes the object in a binary format on the disk
    f.close();
}

void read()
{
    ifstream f("details.db", ios::binary); //Opens file in binary read mode
    Student s;

    while(f.read(reinterpret_cast<char*> (&s), sizeof(s))) s.info(); // Reads the binary file one obj at a time and displays it as normal text 
    f.close();
}

bool update(int id, string addr)
{
    bool found = false;

    fstream f("details.db", ios::in | ios::out | ios::binary); // Opens file in both read write and binary mode
    Student s;

    while(f.read(reinterpret_cast<char*> (&s), sizeof(s)))
    {
        if(s.id == id) //Searching using linear search
        {
            found = true;

            strcpy(s.addr, addr.c_str()); //updates address
            s.ver += 1; //updates version
            s.setTimestamp(); // Refresh timestamp for the update transaction

            
            //f.seekp(f.tellg() - sizeof(s), ios::beg); => can be used to overwite the updates
            // tellg() gets the current position of read pointer and then seekp() moves the write pointer to the correct position

            /*
            tellg - Gives the current position of read pointer
            tellp - Gives the current position of write pointer
            seekg - Moves the read pointer
            seekp - Moves the write pointer
            */ 
            
            f.seekp(0, ios::end); // Moves write pointer to end of file to append the updates                                       
            f.write(reinterpret_cast<char*> (&s), sizeof(s));  

            break;
        }
    }
    return found;
}

bool del(int id)
{
    bool found = false;

    fstream f("details.db", ios::in | ios::out | ios::binary); // Opens file in both read write and binary mode
    Student s;

    while(f.read(reinterpret_cast<char*> (&s), sizeof(s)))
    {
        if(s.id == id) //Searching using linear search
        {
            found = true;
            string d = "DELETED";
            s.flag = 0;
            strcpy(s.name, d.c_str());
            strcpy(s.addr, d.c_str());
            s.ver += 1; //updates version
            s.setTimestamp(); //Refresh timestamp for the deletion transaction
            
            f.seekp(0, ios::end); // Moves write pointer to end of file to append deletion tombstone                                       
            f.write(reinterpret_cast<char*> (&s), sizeof(s));  

            break;
        }
    }
    return found;
}

int main()
{
    Student a(101, 1, "Raj", "Mumbai");
    Student b(102, 1, "Soham", "New Delhi");
    Student c(103, 1, "Ketan", "Pune");

    insert(a); //Inserting 3 students data
    insert(b);
    insert(c);
    cout<<"After inserting 3 students data: "<<endl;
    read();

    cout<<endl;

    update(102, "Bangalore"); //Updates address
    cout<<"After updating: "<<endl;
    read();

    cout<<endl;

    del(101); // Deletes S_ID = 1 i.e. appends a deletion tombstone
    cout<<"After deletion: "<<endl;
    read(); //Reading the db

    return 0;
}