#pragma once
#include "../storage/table.h"
#include "vector_meta.h"

class vecTable{
    private:
    VectorMeta meta;

    public:
    vecTable(const VectorMeta& metadata);
    bool insert(std::string id, uint64_t timestamp, std::vector<float> embed);
};