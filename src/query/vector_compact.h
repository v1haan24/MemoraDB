#pragma once
#include "../storage/table.h"
#include "../vector/vecTable.h"
#include "../vector/semantic/vector_index.h"

bool compactTable(Table& table, vecTable& vt, VectorIndex& idx, uint64_t timestamp);