/* ofstream file("test.db",ios::binary);
    int age=19;
    file.write((char *)&age,sizeof(age)); // two args : char arr of bytes, size

    char arrays of bytes can be made form any class, expale shows for employee name,id, salary and role.

    file.close();

    ifstream infile("test.db",ios::binary);
    int p;
    infile.read((char*)&p,sizeof(p)); // two args : where to write ,how many bytes/length
    cout<<p;
    infile.close();


    C-style cast : (char *)
      CPP :  reinterpret_cast<char*>(&len)   while writing you can also use const char* instead of just char*
*/

#include<iostream>
#include<fstream>
using namespace std;
class employee{
    int id, salary;
    bool onsite;
public:
    employee(int i,int sal,bool os){
        id=i; salary=sal;onsite=os;
    }
};
int main(){
    employee e(26,26000,false);
    ofstream file("employee_data.db",ios::binary); //string not supported, class should be of primitive datatype.
    file.write(reinterpret_cast<char*>(&e),sizeof(e));
    return 0;
}