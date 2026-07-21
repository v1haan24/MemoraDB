#include "../storage/table.h"
using namespace std;

vector<Record> Table::showHistory(const string& pk){
    const vector<RecordVersion>& hist=history.getHistory(pk);
    vector<Record> ans; ans.reserve(hist.size());
    for(const auto& version:hist) ans.push_back(readRecord(version.offset));
    return ans;
}