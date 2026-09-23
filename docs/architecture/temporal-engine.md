# Temporal Engine Architecture

The temporal engine manages record versioning, point-in-time indexing, historical diffing, and state restoration.

---

## Source Location

* **Index**: `src/index/history_index.h`, `src/index/history_index.cpp`
* **Implementations**:
  * `src/temporal/as_of.cpp` — Single-row and key-level `AS OF` resolution.
  * `src/temporal/between.cpp` — Range-based version extraction.
  * `src/temporal/compare.cpp` — Net endpoint comparison and pairwise difference algorithms.
  * `src/temporal/evolution.cpp` — Step-by-step changelog generation.
  * `src/temporal/history.cpp` — Complete version timeline queries.
  * `src/temporal/rollback.cpp` — Non-destructive historical restoration.
  * `src/temporal/snapshot.cpp` — Table-wide point-in-time reconstruction.

---

## The In-Memory `HistoryIndex`

The `HistoryIndex` serves as the temporal index for MemoraDB:

```cpp
class HistoryIndex {
private:
    std::unordered_map<std::string, std::vector<RecordVersion>> history;
public:
    bool contains(const std::string& pk);
    void addVersion(const std::string& pk, const RecordVersion& version);
    const std::vector<RecordVersion>& getHistory(const std::string& pk);
    const RecordVersion& latest(const std::string& pk);
    const RecordVersion* latestBefore(const std::string& pk, uint64_t timestamp);
};
```

### Binary Search via `latestBefore`
Because versions are strictly appended in ascending chronological order, point-in-time queries run in `O(log V)` time using binary search:

```cpp
const RecordVersion* HistoryIndex::latestBefore(const std::string& pk, uint64_t timestamp) {
    auto it = history.find(pk);
    if (it == history.end()) return nullptr;
    
    const std::vector<RecordVersion>& hist = it->second;
    const RecordVersion* ans = nullptr;
    std::ptrdiff_t l = 0, r = static_cast<std::ptrdiff_t>(hist.size()) - 1;
    while (l <= r) {
        std::ptrdiff_t mid = l + (r - l) / 2;
        if (hist[mid].timestamp <= timestamp) {
            ans = &hist[mid];
            l = mid + 1; // Look for a later version that is still <= timestamp
        } else {
            r = mid - 1;
        }
    }
    return ans;
}
```

---

## Temporal Operation Algorithms

### 1. Snapshot Reconstruction (`Table::snapshot`)
1. Iterates over all known primary keys in `HistoryIndex`.
2. For each key, calls `latestBefore(pk, timestamp)`.
3. If the anchor version has `deleted == 0`, reads the record from disk and includes it.

### 2. Net Difference (`Table::compare`)
1. Calls `selectBetween(pk, t1, t2)` to gather all versions in the time window.
2. If empty, returns no differences.
3. Calls `compareRecords(recs.front(), recs.back(), meta)`.
4. Returns only the net attribute differences between the start and end of the window.

### 3. Step-by-Step Changelog (`Table::evolution`)
1. Locates the anchor version at `t1`.
2. Loops through every consecutive version pair `(hist[i], hist[i+1])` up to `t2`.
3. Calls `compareRecords(r1, r2, meta)` on each pair.
4. Stamps each difference with the exact timestamp when that specific change was committed (`diff.timestamp = r2.timestamp`).

### 4. Non-Destructive Rollback (`Table::rollback`)
1. Fetches the historical record via `selectAsOf(pk, timestamp)`.
2. Calls `appendRecord(record)`, creating a brand new record at the end of `data.db` with `timestamp = now()`.
3. The historical state is restored as the current state without destroying subsequent history.
