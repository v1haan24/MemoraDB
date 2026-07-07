#include<iostream>
#include<fstream>
#include<cstring> //required string fucntions
#include<stdio.h> //for del
using namespace std;

class employee{
public:
    int id;
    int salary;
    bool onsite;
    string name;
    string title;

    employee(){}

    employee(int i,int sal,bool os,string n,string t){
        id=i;
        salary=sal;
        onsite=os;
        name=n;
        title=t;
    }

    void display(){
        cout<<"----- Employee Details -----\n";
        cout<<"ID      : "<<id<<endl;
        cout<<"Name    : "<<name<<endl;
        cout<<"Title   : "<<title<<endl;
        cout<<"Salary  : "<<salary<<endl;
        cout<<"Onsite  : "<<(onsite?"Yes":"No")<<endl;
    }
};

void writeEmployee(ofstream& file,const employee& e){
    file.write(reinterpret_cast<const char*>(&e.id),sizeof(e.id));
    file.write(reinterpret_cast<const char*>(&e.salary),sizeof(e.salary));
    file.write(reinterpret_cast<const char*>(&e.onsite),sizeof(e.onsite));

    int len=e.name.size();
    file.write(reinterpret_cast<const char*>(&len),sizeof(len));
    file.write(e.name.c_str(),len);

    len = e.title.size();
    file.write(reinterpret_cast<const char*>(&len),sizeof(len));
    file.write(e.title.c_str(),len);
}

bool readEmployee(ifstream& file, employee& e){
    if(!file.read(reinterpret_cast<char*>(&e.id), sizeof(e.id))) return false;

    file.read(reinterpret_cast<char*>(&e.salary),sizeof(e.salary));
    file.read(reinterpret_cast<char*>(&e.onsite),sizeof(e.onsite));

    int len;
    file.read(reinterpret_cast<char*>(&len),sizeof(len));
    e.name.resize(len);
    file.read(&e.name[0],len);

    file.read(reinterpret_cast<char*>(&len),sizeof(len));
    e.title.resize(len);
    file.read(&e.title[0],len);

    return true;
}

void read(){
    ifstream file("memora.db",ios::binary);
    employee e;
    while(readEmployee(file,e)){
        e.display();
    }
}

int main(){
    ofstream file("memora.db",ios::binary);
    employee e1(26,26000,false,"Vihaan","SDE");
    employee e2(25,70000,true,"Jai","SWE");
    writeEmployee(file,e1);
    writeEmployee(file,e2);
    file.close();    
    read();
    return 0;
}