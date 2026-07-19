#include "history_index.h"

bool HistoryIndex::contains(const string& pk){ return history.count(pk); }

void HistoryIndex::addVersion(const string& pk,const RecordVersion& version){ history[pk].push_back(version); }

vector<RecordVersion>& HistoryIndex::getHistory(const string& pk){ return history.at(pk); }

const RecordVersion& HistoryIndex::latest(const string& pk){ return history.at(pk).back(); }

int HistoryIndex::size(){ return history.size(); }