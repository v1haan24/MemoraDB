#pragma once

#define cns 30 //fixed col name size
#define tns 30 //fixed table name size
#define rhsz (sizeof(uint64_t)+sizeof(bool)) //row-header-size
enum DataType {INT,FLOAT,STRING,BOOL};