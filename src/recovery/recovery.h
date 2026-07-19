#pragma once
#include "../common/metadata.h"
#include "../index/history_index.h"

class Recovery{
public:
    void recoverState(TableMeta& meta,HistoryIndex& history);
};