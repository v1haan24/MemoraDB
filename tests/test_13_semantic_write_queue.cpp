#include <iostream>
#include <cassert>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <vector>
#include "../src/catalog/catalog.h"

using namespace std;
namespace fs = std::filesystem;

static TableMeta makeDocsMeta(){
    TableMeta docs;
    strncpy(docs.name,"QueueDocs",tns-1);
    docs.name[tns-1]='\0';

    docs.columns.push_back(ColMeta("docId",STRING,true,16));

    ColMeta title("title",STRING,false,40);
    title.isSemantic=true;
    docs.columns.push_back(title);

    docs.columns.push_back(ColMeta("category",STRING,false,20));
    return docs;
}

static vector<pair<string,uint64_t>> readDet(const string& path,int& count){
    vector<pair<string,uint64_t>> rows;
    ifstream file(path,ios::binary);
    assert(file.good());

    readBinary(file,count);

    for(int i=0;i<count;i++){
        string pk;
        uint64_t ts=0;
        readString(file,pk);
        readBinary(file,ts);
        assert(file.good());
        rows.push_back({pk,ts});
    }
    return rows;
}

static vector<string> readTasks(const string& path){
    vector<string> lines;
    ifstream file(path);

    assert(file.good());

    string line;
    while(getline(file,line)){
        if(!line.empty()) lines.push_back(line);
    }
    return lines;
}

static void semanticColumnTest(Table* table){
    cout<<"[1] writeQueue() writes only semantic columns...\n";

    Row row={{"doc-001","Database systems use indexes","academic"}};
    assert(table->writeQueue("doc-001",1111,row));

    string dir="data/QueueDocs/queue";
    string det=dir+"/temp_det.queue";
    string tasks=dir+"/temp_tasks.queue";

    assert(fs::exists(det));
    assert(fs::exists(tasks));

    int count=0;
    auto records=readDet(det,count);
    assert(count==1);
    assert(records.size()==1);
    assert(records[0].first=="doc-001");
    assert(records[0].second==1111);

    auto lines=readTasks(tasks);
    assert(lines.size()==1);
    assert(lines[0]=="title: Database systems use indexes ");

    // The non-semantic category must not appear in the embedding text.
    assert(lines[0].find("academic")==string::npos);
}

static void multipleAppendTest(Table* table){
    cout<<"[2] writeQueue() appends records instead of overwriting...\n";

    assert(table->writeQueue(
        "doc-002",2222,
        {{"doc-002","Vector databases store embeddings","technical"}}
    ));

    assert(table->writeQueue(
        "doc-003",3333,
        {{"doc-003","Semantic search finds related meaning","technical"}}
    ));

    string dir="data/QueueDocs/queue";

    int count=0;
    auto records=readDet(dir+"/temp_det.queue",count);
    assert(count==3);
    assert(records.size()==3);

    assert(records[0].first=="doc-001");
    assert(records[1].first=="doc-002");
    assert(records[2].first=="doc-003");

    auto lines=readTasks(dir+"/temp_tasks.queue");
    assert(lines.size()==3);
    assert(lines[1]=="title: Vector databases store embeddings ");
    assert(lines[2]=="title: Semantic search finds related meaning ");
}

static void noSemanticColumnTest(){
    cout<<"[3] writeQueue() rejects tables without semantic columns...\n";

    fs::remove_all("data/NoSemanticQueue");

    Catalog catalog;
    TableMeta t;
    strncpy(t.name,"NoSemanticQueue",tns-1);
    t.name[tns-1]='\0';
    t.columns.push_back(ColMeta("id",STRING,true,10));
    t.columns.push_back(ColMeta("value",STRING,false,20));

    assert(catalog.createTable(t));

    Table* table=catalog.getTable("NoSemanticQueue");
    assert(table!=nullptr);

    assert(!table->writeQueue("x",100,{{"x","hello"}}));
    assert(!fs::exists("data/NoSemanticQueue/queue"));
}

static void semanticOrderTest(){
    cout<<"[4] Semantic columns preserve table-column order...\n";

    fs::remove_all("data/OrderQueue");

    Catalog catalog;
    TableMeta t;
    strncpy(t.name,"OrderQueue",tns-1);
    t.name[tns-1]='\0';

    t.columns.push_back(ColMeta("id",STRING,true,10));

    ColMeta first("first",STRING,false,20);
    first.isSemantic=true;
    t.columns.push_back(first);

    t.columns.push_back(ColMeta("ignored",STRING,false,20));

    ColMeta second("second",STRING,false,20);
    second.isSemantic=true;
    t.columns.push_back(second);

    assert(catalog.createTable(t));

    Table* table=catalog.getTable("OrderQueue");
    assert(table!=nullptr);
    assert(table->writeQueue(
        "a",4444,
        {{"a","hello","NOT_INCLUDED","world"}}
    ));

    auto lines=readTasks("data/OrderQueue/queue/temp_tasks.queue");
    assert(lines.size()==1);
    assert(lines[0]=="first: hello second: world ");
}

int main(){
    cout<<"\n======= Semantic writeQueue() Tests =======\n\n";

    fs::remove_all("data/QueueDocs");

    Catalog catalog;
    TableMeta docs=makeDocsMeta();
    assert(catalog.createTable(docs));

    Table* table=catalog.getTable("QueueDocs");
    assert(table!=nullptr);

    semanticColumnTest(table);
    multipleAppendTest(table);
    noSemanticColumnTest();
    semanticOrderTest();

    cout<<"\n===========================================\n";
    cout<<"All writeQueue() tests passed.\n";
    cout<<"===========================================\n";
    return 0;
}
