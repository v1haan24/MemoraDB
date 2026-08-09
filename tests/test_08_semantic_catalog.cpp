#include <iostream>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <filesystem>
#include "../src/semantic/vecTable.h"
#include "../src/semantic/vector_meta.h"
using namespace std;

void setupDocsMeta(TableMeta& docs){
    strncpy(docs.name,"Docs",tns-1);
    docs.name[tns-1]='\0';
    docs.columns.push_back(ColMeta("docId",STRING,true,16)); // fixed-width primary key
    docs.columns.push_back(ColMeta("title",STRING,false,30)); // ignored by vecMeta, present to mimic a real table
}

void createVecTableTest(vecMeta& vm,TableMeta& docs,VectorMeta& meta){
    cout<<"[1] Creating vector table...\n";
    assert(vm.createVecTable(meta,docs));
    ifstream file("data/Docs/Docs.vec",ios::binary);
    assert(file.good());
}

void metadataFieldsTest(const VectorMeta& meta){
    cout<<"[2] Metadata field verification...\n";
    assert(strcmp(meta.name,"Vector_DB_Docs")==0);
    assert(meta.pkSize==16);
    assert(meta.recordCount==0);
    int expectedPayload=sizeof(uint32_t)+16+sizeof(uint64_t)+sizeof(float)*VEC_DIM;
    assert(meta.payloadSize==expectedPayload);
    int expectedHeader=sizeof(int)+tns+sizeof(int)+sizeof(uint32_t)+sizeof(int);
    assert(meta.metadataSize==expectedHeader);
}

void duplicateCreateTest(vecMeta& vm,TableMeta& docs){
    cout<<"[3] Duplicate vector table detection...\n";
    VectorMeta again;
    assert(!vm.createVecTable(again,docs));
}

void noPrimaryKeyTest(vecMeta& vm){
    cout<<"[4] Rejects table with no primary key...\n";
    TableMeta noPk;
    strncpy(noPk.name,"NoPk",tns-1); noPk.name[tns-1]='\0';
    noPk.columns.push_back(ColMeta("label",STRING,false,10));
    VectorMeta m;
    assert(!vm.createVecTable(m,noPk));
    assert(!filesystem::exists("data/NoPk/NoPk.vec"));
}

void multiplePrimaryKeyTest(vecMeta& vm){
    cout<<"[5] Rejects table with more than one primary key...\n";
    TableMeta twoPk;
    strncpy(twoPk.name,"TwoPk",tns-1); twoPk.name[tns-1]='\0';
    twoPk.columns.push_back(ColMeta("a",STRING,true,8));
    twoPk.columns.push_back(ColMeta("b",STRING,true,8));
    VectorMeta m;
    assert(!vm.createVecTable(m,twoPk));
}

void zeroSizePrimaryKeyTest(vecMeta& vm){
    cout<<"[6] Rejects zero-size STRING primary key...\n";
    TableMeta zeroPk;
    strncpy(zeroPk.name,"ZeroPk",tns-1); zeroPk.name[tns-1]='\0';
    zeroPk.columns.push_back(ColMeta("a",STRING,true)); // no size given -> size=0
    VectorMeta m;
    assert(!vm.createVecTable(m,zeroPk));
}

void readMetadataTest(vecMeta& vm,TableMeta& docs,const VectorMeta& original){
    cout<<"[7] Reading metadata back from disk...\n";
    VectorMeta reloaded=vm.readMetadata("Docs.vec",docs);
    assert(strcmp(reloaded.name,original.name)==0);
    assert(reloaded.pkSize==original.pkSize);
    assert(reloaded.recordCount==original.recordCount);
    assert(reloaded.payloadSize==original.payloadSize);
    assert(reloaded.metadataSize==original.metadataSize);
}

int main(){
    filesystem::remove_all("data/Docs");
    filesystem::remove_all("data/NoPk");
    filesystem::remove_all("data/TwoPk");
    filesystem::remove_all("data/ZeroPk");

    cout<<"\n========== Semantic Catalog Tests ==========\n\n";

    vecMeta vm;
    TableMeta docs;
    VectorMeta meta;
    setupDocsMeta(docs);

    createVecTableTest(vm,docs,meta);
    metadataFieldsTest(meta);
    duplicateCreateTest(vm,docs);
    noPrimaryKeyTest(vm);
    multiplePrimaryKeyTest(vm);
    zeroSizePrimaryKeyTest(vm);
    readMetadataTest(vm,docs,meta);

    cout<<"\n=============================================\n";
    cout<<"All semantic catalog tests passed.\n";
    cout<<"=============================================\n";
    return 0;
}
