#pragma once
#include "../storage/table.h"
#include "vector_meta.h"

class vecTable{
    private:
    VectorMeta meta;

    public:
    vecTable(const VectorMeta& metadata);
    bool insert(std::string pk, uint64_t timestamp, const float (&embed)[VEC_DIM]);
    VecRecord readRecord(uint32_t id);

    VectorMeta& getMeta(){ return meta; }
};