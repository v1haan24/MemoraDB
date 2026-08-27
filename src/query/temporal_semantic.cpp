#include "temporal_semantic_search.h"

std::vector<SearchResult> temporalSemanticSearch(
    Table& table,
    VectorIndex& idx,
    CandidateMode mode,
    uint64_t t1,
    uint64_t t2,
    const float (&queryEmbedding)[VEC_DIM],
    int k,
    const WhereClause* clause){

    std::vector<VCandidate> refined;

    if(clause == nullptr){
        refined = generateCandidateKeys(table, mode, t1, t2);
    }
    else{
        std::vector<Record> candidates = generateCandidates(table, mode, t1, t2);
        candidates = where(candidates, table.getMeta(), *clause);
        refined = refineCandidates(candidates, table.getMeta());
    }
    return idx.semanticSearch(refined, queryEmbedding, k);
}