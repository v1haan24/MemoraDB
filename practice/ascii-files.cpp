/*write
    ofstream file;
    file.open("test.txt",ios::app); //ios::app is appending flag prevents overwrite
    //or : offstream file("test.txt"); overwritten everytime
    //write to file
    file<<"Hello Memora\n";
    file.close();
*/

/*read file
    ifstream infile("test.txt");
    string s; //for first char : char s

    //infile>>s; reads one word ( breaks at whitespace)
    //getline(infile,s); read o ne sentence

    while(getline(infile,s,'!')){ //eof-> end of file !infile.eof(), third arg is delimitter
        cout<<s;
    }

    infile.close();
*/

#include<iostream>
#include<fstream>
using namespace std;

int main(){
    int age;
    string name;
    cout<<"Enter your name"<<endl;
    cin>>name;
    cout<<"Enter your age"<<endl;
    cin>>age;

    ofstream file("details.txt");
    file<<"Name:"<<name<<endl;
    file<<"Age:"<<age<<endl;
    file.close();

    ifstream rfile("details.txt");
    string s;
    cout<<"-----------YOUR DETAILS---------------"<<endl;
    while(getline(rfile,s,'\n')) cout<<s<<endl;
    rfile.close();
    return 0;
}