#include <iostream>
#include <cassert>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <thread>
#include <vector>
#include <unordered_set>
#include "../src/catalog/catalog.h"

using namespace std;
namespace fs = std::filesystem;

static TableMeta makeMeta(){
    TableMeta t;
    strncpy(t.name,"ConcurrentQueue",tns-1);
    t.name[tns-1]='\0';

    t.columns.push_back(ColMeta("id",STRING,true,32));

    ColMeta text("text",STRING,false,80);
    text.isSemantic=true;
    t.columns.push_back(text);

    return t;
}

static vector<pair<string,uint64_t>> readDet(const string& path,int& count){
    vector<pair<string,uint64_t>> rows;
    ifstream file(path,ios::binary);
    assert(file.good());

    readBinary(file,count);
    assert(count>=0);

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

int main(){
    cout<<"\n======= writeQueue() Concurrency Tests =======\n\n";

    fs::remove_all("data/ConcurrentQueue");

    Catalog catalog;
    TableMeta meta=makeMeta();
    assert(catalog.createTable(meta));

    Table* table=catalog.getTable("ConcurrentQueue");
    assert(table!=nullptr);

    const int THREADS=8;
    const int PER_THREAD=25;
    const int EXPECTED=THREADS*PER_THREAD;

    vector<thread> workers;
    workers.reserve(THREADS);

    for(int t=0;t<THREADS;t++){
        workers.emplace_back([table,t](){
            for(int i=0;i<PER_THREAD;i++){
                string pk="pk-"+to_string(t)+"-"+to_string(i);
                string text="semantic text from thread "+to_string(t)+
                            " record "+to_string(i);

                uint64_t timestamp=
                    1000000ULL+
                    static_cast<uint64_t>(t*PER_THREAD+i);

                bool ok=table->writeQueue(
                    pk,
                    timestamp,
                    {{pk,text}}
                );

                assert(ok);
            }
        });
    }

    for(auto& worker:workers) worker.join();

    cout<<"[1] All concurrent writeQueue() calls returned successfully.\n";

    string dir="data/ConcurrentQueue/queue";
    int count=0;
    auto records=readDet(dir+"/temp_det.queue",count);

    cout<<"[2] Queue count = "<<count<<"\n";
    assert(count==EXPECTED);
    assert(static_cast<int>(records.size())==EXPECTED);

    unordered_set<string> seen;
    for(const auto& record:records){
        assert(!record.first.empty());
        assert(seen.insert(record.first).second);
    }

    cout<<"[3] No queue records were lost or duplicated.\n";

    ifstream tasks(dir+"/temp_tasks.queue");
    assert(tasks.good());

    int lineCount=0;
    string line;
    while(getline(tasks,line)){
        if(!line.empty()){
            lineCount++;
            assert(line.find("semantic text from thread ")==0);
        }
    }

    cout<<"[4] Task line count = "<<lineCount<<"\n";
    assert(lineCount==EXPECTED);

    cout<<"\n==============================================\n";
    cout<<"All concurrent writeQueue() tests passed.\n";
    cout<<"==============================================\n";
    return 0;
}
