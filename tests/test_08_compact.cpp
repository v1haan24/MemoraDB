#include <iostream>
#include <cassert>
#include <filesystem>
#include "../src/catalog/catalog.h"
using namespace std;

int main(){
    filesystem::remove_all("data/Student");

    {
        Catalog catalog;
        TableMeta student;
        strncpy(student.name,"Student",tns-1);
        student.name[tns-1]='\0';
        student.columns.push_back({"rollNo",INT,true});
        student.columns.push_back({"name",STRING,false,30});
        assert(catalog.createTable(student));

        Table* table=catalog.getTable("Student");
        assert(table!=nullptr);
        assert(table->insert({{"101","Swayam"}}));
        assert(table->update({{"101","Swayam Jain"}}));

        assert(table->compact(1754763201));
        assert(filesystem::exists("data/Student/data.db"));
        assert(filesystem::exists("data/Student/archive/archive_1754763201.db"));

        assert(table->compact(1754763202));
        assert(filesystem::exists("data/Student/archive/archive_1754763202.db"));
        assert(!table->compact(1754763202));
    }

    Catalog restarted;
    Table* table=restarted.getTable("Student");
    assert(table!=nullptr);
    assert(table->latest("101").row.values[1]=="Swayam Jain");

    cout<<"All compaction tests passed.\n";
    return 0;
}
