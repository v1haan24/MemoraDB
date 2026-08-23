#pragma once
#include "query.h"
#include "../vector/vecTable.h"
#include "../vector/semantic/candidate_bridge.h"
#include "../vector/semantic/vector_index.h"

std::vector<SearchResult> temporalSemanticSearch(
    Table& table,
    VectorIndex& idx,
    CandidateMode mode,
    uint64_t t1,
    uint64_t t2,
    const float (&queryEmbedding)[VEC_DIM],
    int k,
    const WhereClause* clause = nullptr
);
