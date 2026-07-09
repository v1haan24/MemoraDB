#include <iostream>
#include "catalog.h"
using namespace std;

int main(){
    Catalog catalog;

    TableMeta employee;
    strcpy(employee.name,"Employee");
    employee.columns.push_back({"id",INT,true});
    employee.columns.push_back({"name",STRING,false,21});
    employee.columns.push_back({"salary",FLOAT,false});
    employee.columns.push_back({"active",BOOL,false});
    catalog.createTable(employee);

    TableMeta student;
    strcpy(student.name,"Student");
    student.columns.push_back({"rollNo",INT,true});
    student.columns.push_back({"studentName",STRING,false,31});
    student.columns.push_back({"cgpa",FLOAT,false});
    student.columns.push_back({"hosteller",BOOL,false});
    catalog.createTable(student);

    TableMeta course;
    strcpy(course.name,"Course");
    course.columns.push_back({"courseId",INT,true});
    course.columns.push_back({"courseName",STRING,false,41});
    course.columns.push_back({"credits",INT,false});
    catalog.createTable(course);

    cout<<"All tables created.\n\n";

    TableMeta emp=catalog.readMetadata("Employee.db");
    cout<<emp.name<<" Row Size : "<<emp.rowSize<<endl;

    TableMeta stu=catalog.readMetadata("Student.db");
    cout<<stu.name<<" Row Size : "<<stu.rowSize<<endl;

    TableMeta cou=catalog.readMetadata("Course.db");
    cout<<cou.name<<" Row Size : "<<cou.rowSize<<endl;

    return 0;
}