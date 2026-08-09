#include <iostream>
#include <cassert>
#include <cstring>
#include <filesystem>
#include "../src/semantic/vecTable.h"
#include "../src/semantic/vector_meta.h"
using namespace std;

// NOTE: The four records inserted here (pk, timestamp, embedding base value)
// are the fixed "known good" dataset. test_12_semantic_read.cpp does not
// share memory with this process -- it re-derives the exact same expected
// values independently and checks them against what's actually on disk.
// If you ever change the data inserted below, update test_12 to match.

void makeEmbedding(float (&e)[VEC_DIM],float base){
    for(int i=0;i<VEC_DIM;i++) e[i]=base+0.001f*i;
}

TableMeta setupDocsMeta(){
    TableMeta docs;
    strncpy(docs.name,"Docs",tns-1);
    docs.name[tns-1]='\0';
    docs.columns.push_back(ColMeta("docId",STRING,true,16)); // fixed-width primary key
    return docs;
}

int main(){
    filesystem::remove_all("data/Docs"); // start from a clean slate

    cout<<"\n========== Semantic Create Tests ==========\n\n";

    cout<<"[1] Creating vector table 'Docs'...\n";
    TableMeta docs=setupDocsMeta();
    vecMeta vm;
    VectorMeta meta;
    assert(vm.createVecTable(meta,docs));
    ifstream check("data/Docs/Docs.vec",ios::binary);
    assert(check.good());
    check.close();

    cout<<"[2] Inserting known records...\n";
    vecTable vt(meta);

    float e0[VEC_DIM]; makeEmbedding(e0,0.5f);
    float e1[VEC_DIM]; makeEmbedding(e1,1.5f);
    float e2[VEC_DIM]; makeEmbedding(e2,2.5f);
    float e3[VEC_DIM]; makeEmbedding(e3,3.5f);

    assert(vt.insert("alpha",1000,e0));
    assert(vt.insert("bravo",2000,e1));
    assert(vt.insert("charlie1234567",3000,e2)); // 14 chars, under the 16-byte limit
    assert(vt.insert("delta-16-chars!!",4000,e3)); // exactly 16 chars

    cout<<"[3] Verifying record count on disk...\n";
    assert(vt.getMeta().recordCount==4);

    cout<<"\n=============================================\n";
    cout<<"Vector DB created with 4 records. Run\n";
    cout<<"test_12_semantic_read next to verify them.\n";
    cout<<"=============================================\n";
    return 0;
}
