#pragma once
#include <cstdint>

#define cns 30 //fixed col name size
#define tns 30 //fixed table name size
#define rhsz (sizeof(uint64_t)+sizeof(uint8_t)) //row-header-size
#define VEC_DIM 384 //fixed vector dimension