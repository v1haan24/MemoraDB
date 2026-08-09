#include <iostream>
#include <cassert>
#include <cstring>
#include <filesystem>
#include "../src/semantic/vecTable.h"
#include "../src/semantic/vector_meta.h"
using namespace std;

// NOTE: This file does NOT create or insert anything. It must be run
// AFTER test_11_semantic_create, in a separate process, and reads back
// exactly the "data/Docs/Docs.vec" file that process left on disk.
// The expected pk/timestamp/embedding values below are hardcoded to
// match what test_11_semantic_create inserts -- keep the two in sync.

void makeEmbedding(float (&e)[VEC_DIM],float base){
    for(int i=0;i<VEC_DIM;i++) e[i]=base+0.001f*i;
}

bool embeddingEqual(const float (&a)[VEC_DIM],const float (&b)[VEC_DIM]){
    for(int i=0;i<VEC_DIM;i++) if(a[i]!=b[i]) return false;
    return true;
}

TableMeta setupDocsMeta(){
    TableMeta docs;
    strncpy(docs.name,"Docs",tns-1);
    docs.name[tns-1]='\0';
    docs.columns.push_back(ColMeta("docId",STRING,true,16));
    return docs;
}

void fileExistsTest(){
    cout<<"[1] Vector DB file exists on disk...\n";
    ifstream file("data/Docs/Docs.vec",ios::binary);
    assert(file.good());
}

VectorMeta metadataTest(vecMeta& vm,TableMeta& docs){
    cout<<"[2] Reading metadata from disk...\n";
    VectorMeta meta=vm.readMetadata("Docs.vec",docs);
    assert(strcmp(meta.name,"Vector_DB_Docs")==0);
    assert(meta.pkSize==16);
    assert(meta.recordCount==4);
    int expectedPayload=sizeof(uint32_t)+16+sizeof(uint64_t)+sizeof(float)*VEC_DIM;
    assert(meta.payloadSize==expectedPayload);
    return meta;
}

void recordsTest(vecTable& vt){
    cout<<"[3] Reading back all records...\n";

    float e0[VEC_DIM]; makeEmbedding(e0,0.5f);
    VecRecord r0=vt.readRecord(0);
    assert(r0.pk=="alpha");
    assert(r0.timestamp==1000);
    assert(embeddingEqual(r0.embedding,e0));

    float e1[VEC_DIM]; makeEmbedding(e1,1.5f);
    VecRecord r1=vt.readRecord(1);
    assert(r1.pk=="bravo");
    assert(r1.timestamp==2000);
    assert(embeddingEqual(r1.embedding,e1));

    float e2[VEC_DIM]; makeEmbedding(e2,2.5f);
    VecRecord r2=vt.readRecord(2);
    assert(r2.pk=="charlie1234567");
    assert(r2.timestamp==3000);
    assert(embeddingEqual(r2.embedding,e2));

    float e3[VEC_DIM]; makeEmbedding(e3,3.5f);
    VecRecord r3=vt.readRecord(3);
    assert(r3.pk=="delta-16-chars!!");
    assert(r3.timestamp==4000);
    assert(embeddingEqual(r3.embedding,e3));
}

void outOfRangeTest(vecTable& vt){
    cout<<"[4] Reading a non-existent record fails gracefully...\n";
    VecRecord bad=vt.readRecord(999);
    assert(bad.pk.empty());
    assert(bad.timestamp==0);
}

int main(){
    cout<<"\n========== Semantic Read Tests ==========\n\n";

    fileExistsTest();

    TableMeta docs=setupDocsMeta();
    vecMeta vm;
    VectorMeta meta=metadataTest(vm,docs);

    vecTable vt(meta);
    recordsTest(vt);
    outOfRangeTest(vt);

    cout<<"\n==========================================\n";
    cout<<"All semantic read tests passed.\n";
    cout<<"==========================================\n";
    return 0;
}
