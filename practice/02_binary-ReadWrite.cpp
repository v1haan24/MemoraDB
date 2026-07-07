#include<iostream>
#include<fstream>
#include<cstring>
using namespace std;


class Student
{
    public:

    int id, age;
    char name[50];

    Student() {}

    Student(int i, string nam, int a)
    {
        id=i;
        age = a;
        strcpy(name, nam.c_str());
    }
};


int main()
{
    //WRITE
    Student s(10, "Naman", 19);

    ofstream file("student_data.db", ios::binary); //Opened file in binary mode to write object
    file.write(reinterpret_cast<char*>(&s),sizeof(s)); // two args : a pointer to a character buffer, no. of bytes to write

    file.close();

    //READ
    Student s1;

    ifstream f("student_data.db", ios::binary); //Opened file in binary mode to read object
    f.read(reinterpret_cast<char*>(&s1),sizeof(s1));

    cout<<"-----------YOUR DETAILS---------------"<<endl;
    cout<<"ID: "<<s1.id<<endl;
    cout<<"Name: "<<s1.name<<endl;
    cout<<"Age: "<<s1.age<<endl;



    return 0;
}