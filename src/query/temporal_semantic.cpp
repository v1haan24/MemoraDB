#include "temporal_semantic.h"

std::vector<SearchResult> temporalSemanticSearch(
    Table& table,
    VectorIndex& idx,
    CandidateMode mode,
    uint64_t t1,
    uint64_t t2,
    const float (&queryEmbedding)[VEC_DIM],
    int k,
    const WhereClause* clause){

    std::vector<Record> candidates = generateCandidates(table, mode, t1, t2);

    if(clause != nullptr){
        candidates = where(candidates, table.getMeta(), *clause);
    }

    std::vector<VCandidate> refined = refineCandidates(candidates, table.getMeta());

    return idx.semanticSearch(refined, queryEmbedding, k);
}
