#include "common/constants.h"
#include "catalog/catalog.h"
#include "storage/table.h"
#include "vector/vecTable.h"
#include "query/query.h"
#include <iostream>
#include <vector>


void runSemanticTest(Table& table,vecTable& vectors,CandidateMode mode,uint64_t t1,uint64_t t2,const std::string& testName){
    std::cout<<"\n========================================\n";
    std::cout<<testName<<'\n';
    std::cout<<"========================================\n";

    auto records=generateCandidates(table,mode,t1,t2);
    std::cout<<"Main DB candidates: "<<records.size()<<'\n';

    auto candidates=vectors.refineCandidates(records,table.getMeta());
    std::cout<<"Vector candidates: "<<candidates.size()<<'\n';

    std::cout<<"\nCandidates:\n";
    for(const auto& candidate:candidates){
        std::cout<<"PK: "<<candidate.pk
                 <<" | Timestamp: "<<candidate.timestamp<<'\n';
    }

    VecRecord queryRecord=vectors.readRecord(0);
    auto results=vectors.semanticSearch(candidates,queryRecord.embedding,5);

    std::cout<<"\nSemantic Results:\n";
    for(const auto& result:results){
        std::cout<<"PK: "<<result.pk
                 <<" | Timestamp: "<<result.timestamp
                 <<" | Score: "<<result.score<<'\n';
    }

    if(results.size()!=candidates.size()){
        std::cout<<"\nWARNING: Result count != candidate count!\n";
    }
    else{
        std::cout<<"\nPASS: Every candidate had a vector match.\n";
    }
}


int main(){
    Catalog catalog;

    Table* table=catalog.getTable("SemanticTest");

    if(!table){
        std::cerr<<"SemanticTest table not found.\n";
        return 1;
    }

    std::cout<<"SemanticTest loaded successfully.\n";

    VectorMeta meta;
    meta.tablePath="data/SemanticTest/SemanticTest.vec";
    meta.metadataSize=46;
    meta.pkSize=16;
    meta.payloadSize=1564;
    meta.recordCount=25;

    vecTable vectors(meta);
    vectors.buildIndex();

    runSemanticTest(*table,vectors,LATEST,0,0,"TEST 1: LATEST");
    runSemanticTest(*table,vectors,SNAPSHOT,200002,0,"TEST 2: SNAPSHOT @ 200002");
    runSemanticTest(*table,vectors,BETWEEN,200001,200003,"TEST 3: BETWEEN 200001 -> 200003");
    runSemanticTest(*table,vectors,HISTORY,0,0,"TEST 4: HISTORY");

    std::cout<<"\n\n========================================\n";
    std::cout<<"ALL SEMANTIC + TEMPORAL TESTS FINISHED\n";
    std::cout<<"========================================\n";

    return 0;
}