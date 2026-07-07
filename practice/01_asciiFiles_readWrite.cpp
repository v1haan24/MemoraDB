#include<iostream>
#include<fstream> //file handling reqd
using namespace std;

int main(){
    int id;
    string name;
    
    cout<<"Enter your ID: ";
    cin>>id;
    cout<<"Enter your name: ";
    cin>>name;

    ofstream file("details.txt");
    file<<"ID: "<<id<<endl;
    file<<"Name: "<<name<<endl;
    file.close();

    ifstream f("details.txt");
    string s;
    cout<<"-----------YOUR DETAILS---------------"<<endl;
    while(getline(f,s)) cout<<s<<endl;
    f.close();
    return 0;
}