/*tellg - Where is the read pointer right now?
seekg() - This moves the read pointer.
*/
#include<iostream>
#include<fstream>
#include<cstring> //required string fucntions
#include<stdio.h> //for del
using namespace std;

class employee{
public:
    char name[30],title[30]; //VARCHAR for string, cuz string are object.
    int id, salary;
    bool onsite;
    employee() {}
    employee(int i,int sal,bool os,string n,string t){
        id=i; salary=sal;onsite=os;strcpy(name,n.c_str());strcpy(title,t.c_str());
        //strcpy() instead of individual allotment shortcut
    }
    void display() {
    cout<<"----- Employee Details -----"<<endl;
    cout<<"ID      : "<<id<<endl;
    cout<<"Name    : "<<name<<endl;
    cout<<"Title   : "<<title<<endl;
    cout<<"Salary  : "<<salary<<endl;
    cout<<"Onsite  : "<<(onsite?"Yes":"No")<<endl;
    }
};

void create(employee& e){
    ofstream file("employee_data.db",ios::binary| ios::app);
    file.write(reinterpret_cast<const char*>(&e),sizeof(e));
    file.close();
}

void read(){
    ifstream file("employee_data.db",ios::binary);
    employee temp;
    while(file.read(reinterpret_cast<char*>(&temp),sizeof(temp))){
        temp.display();
    }
    file.close();
}

bool update(int pk,int sal,bool os,string n,string t){
    bool found=false;
    fstream file("employee_data.db",ios::binary | ios::in | ios::out); //to open both in read and write
    employee temp;
    while(file.read(reinterpret_cast<char*>(&temp),sizeof(temp))){
        if(temp.id==pk){
            temp.salary=sal;
            temp.onsite=os;
            strcpy(temp.name,n.c_str());
            strcpy(temp.title,t.c_str());
            file.seekp(file.tellg()-sizeof(temp),ios::beg);
            file.write(reinterpret_cast<const char*>(&temp),sizeof(temp));
            found=true;
            break;
        }
    }
    return found;
}

void del(int pk){
    ifstream file("employee_data.db",ios::binary); 
    ofstream temp("temp.db",ios::binary);
    employee tempe;
    while(file.read(reinterpret_cast<char*>(&tempe),sizeof(tempe))){
        if(tempe.id!=pk){
            temp.write(reinterpret_cast<const char*>(&tempe),sizeof(tempe));
        }
    }
    file.close();
    temp.close();
    remove("employee_data.db");
    rename("temp.db","employee_data.db");
}

int main(){
    employee e1(26,26000,false,"Vihaan","SDE");
    employee e2(25,70000,true,"Jai","SWE");
    create(e1);
    create(e2);
    read();
    update(26,50000,true,"Vihaan Jain","SDE");
    read();
    del(26);
    cout<<"----------FIANL FILE : ------------------"<<endl;
    read();
    return 0;
}