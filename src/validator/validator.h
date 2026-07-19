#pragma once
#include <string>
#include "../common/metadata.h"
#include "../common/row.h"
using namespace std;

class Validator{
public:
    bool validateRow(const Row& row,const TableMeta& meta);
    bool validateValue(const string& value,const ColMeta& col);
};