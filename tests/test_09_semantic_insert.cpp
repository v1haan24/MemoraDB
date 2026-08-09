#include <iostream>
#include <cassert>
#include <cstring>
#include <filesystem>
#include "../src/semantic/vecTable.h"
#include "../src/semantic/vector_meta.h"
using namespace std;

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

void validInsertTest(vecTable& vt){
    cout<<"[1] Valid inserts...\n";
    float e1[VEC_DIM]; makeEmbedding(e1,0.0f);
    float e2[VEC_DIM]; makeEmbedding(e2,1.0f);
    float e3[VEC_DIM]; makeEmbedding(e3,2.0f);

    assert(vt.insert("doc-001",1000,e1));
    assert(vt.insert("doc-002",2000,e2)); 
    assert(vt.insert("doc-003",3000,e3));

    VecRecord r0=vt.readRecord(0);
    assert(r0.pk=="doc-001");
    assert(r0.timestamp==1000);
    assert(embeddingEqual(r0.embedding,e1));

    VecRecord r1=vt.readRecord(1);
    assert(r1.pk=="doc-002");
    assert(r1.timestamp==2000);
    assert(embeddingEqual(r1.embedding,e2));

    VecRecord r2=vt.readRecord(2);
    assert(r2.pk=="doc-003");
    assert(r2.timestamp==3000);
    assert(embeddingEqual(r2.embedding,e3));
}

void recordCountTest(vecTable& vt){
    cout<<"[2] Record count tracking...\n";
    assert(vt.getMeta().recordCount==3);
}

void emptyPrimaryKeyTest(vecTable& vt){
    cout<<"[3] Empty primary key rejected...\n";
    float e[VEC_DIM]; makeEmbedding(e,9.0f);
    assert(!vt.insert("",5000,e));
    assert(vt.getMeta().recordCount==3); // unchanged
}

void oversizedPrimaryKeyTest(vecTable& vt){
    cout<<"[4] Oversized primary key rejected...\n";
    float e[VEC_DIM]; makeEmbedding(e,9.0f);
    assert(!vt.insert("this-primary-key-is-far-too-long",6000,e));
    assert(vt.getMeta().recordCount==3); // unchanged
}

void boundaryPrimaryKeyTest(vecTable& vt){
    cout<<"[5] Primary key exactly at fixed size...\n";
    float e[VEC_DIM]; makeEmbedding(e,4.0f);
    string pk16="exactly16chars!!";
    assert(pk16.size()==16);
    assert(vt.insert(pk16,7000,e));
    VecRecord r=vt.readRecord(3);
    assert(r.pk==pk16);
    assert(r.timestamp==7000);
    assert(embeddingEqual(r.embedding,e));
}

void outOfRangeReadTest(vecTable& vt){
    cout<<"[6] Out-of-range read handled gracefully...\n";
    VecRecord r=vt.readRecord(999);
    assert(r.pk.empty());
    assert(r.timestamp==0);
}

void independentTablesTest(){
    cout<<"[7] Two vector tables stay independent...\n";
    filesystem::remove_all("data/Faqs");
    TableMeta faqs;
    strncpy(faqs.name,"Faqs",tns-1); faqs.name[tns-1]='\0';
    faqs.columns.push_back(ColMeta("faqId",STRING,true,8));

    vecMeta vm;
    VectorMeta faqMeta;
    assert(vm.createVecTable(faqMeta,faqs));
    vecTable faqTable(faqMeta);

    float e[VEC_DIM]; makeEmbedding(e,5.0f);
    assert(faqTable.insert("faq0001",100,e));
    assert(faqTable.getMeta().recordCount==1);

    // Docs.vec (4 records from earlier tests) must be unaffected
    TableMeta docs=setupDocsMeta();
    vecMeta vm2;
    VectorMeta reloadedDocs=vm2.readMetadata("Docs.vec",docs);
    assert(reloadedDocs.recordCount==4);
}

int main(){
    filesystem::remove_all("data/Docs");

    cout<<"\n========== Semantic Insert Tests ==========\n\n";

    TableMeta docs=setupDocsMeta();
    vecMeta vm;
    VectorMeta meta;
    assert(vm.createVecTable(meta,docs));
    vecTable vt(meta);

    validInsertTest(vt);
    recordCountTest(vt);
    emptyPrimaryKeyTest(vt);
    oversizedPrimaryKeyTest(vt);
    boundaryPrimaryKeyTest(vt);
    outOfRangeReadTest(vt);
    independentTablesTest();

    cout<<"\n============================================\n";
    cout<<"All semantic insert tests passed.\n";
    cout<<"============================================\n";
    return 0;
}
