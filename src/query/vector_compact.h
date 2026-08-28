#pragma once
#include "../storage/table.h"
#include "../vector/vecTable.h"

bool compactTable(Table& table, vecTable& vt, uint64_t timestamp);