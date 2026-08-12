#include <iostream>
#include <cassert>
#include <cstring>
#include <filesystem>
#include "../src/catalog/catalog.h"

using namespace std;
namespace fs = std::filesystem;

/*
    Fixture for the full C++ -> queue -> Python -> embedMake -> .vec test.

    This executable only prepares a real MemoraDB table and calls the real
    Table::writeQueue(). The accompanying PowerShell runner then starts:
        1. embedMake.exe
        2. a deterministic Python queue worker
    and verifies the resulting .vec file.

    Keeping the process orchestration outside this test makes the C++ test
    independent of Windows process-management APIs.
*/

int main(){
    cout<<"\n========== Pipeline Fixture ==========\n\n";

    fs::remove_all("data/PipelineDocs");

    Catalog catalog;

    TableMeta docs;
    strncpy(docs.name,"PipelineDocs",tns-1);
    docs.name[tns-1]='\0';

    docs.columns.push_back(ColMeta("docId",STRING,true,16));

    ColMeta text("text",STRING,false,80);
    text.isSemantic=true;
    docs.columns.push_back(text);

    ColMeta category("category",STRING,false,20);
    docs.columns.push_back(category);

    assert(catalog.createTable(docs));

    Table* table=catalog.getTable("PipelineDocs");
    assert(table!=nullptr);

    assert(table->writeQueue(
        "p001",1001,
        {{"p001","database systems and indexing","db"}}
    ));

    assert(table->writeQueue(
        "p002",1002,
        {{"p002","semantic search with embeddings","ml"}}
    ));

    assert(table->writeQueue(
        "p003",1003,
        {{"p003","vector databases and nearest neighbours","vector"}}
    ));

    assert(fs::exists("data/PipelineDocs/queue/temp_tasks.queue"));
    assert(fs::exists("data/PipelineDocs/queue/temp_det.queue"));

    cout<<"Prepared 3 real writeQueue() records.\n";
    cout<<"====================================\n";
    return 0;
}
