# Semantic Engine Architecture

The semantic engine provides native vector embeddings, dedicated vector storage, and metric-space similarity search within MemoraDB.

---

## Source Location

* **Model & Embedding**: `src/vector/minilm_embedder.h`, `src/vector/minilm_embedder.cpp`
* **Vector Storage**: `src/vector/vecTable.h`, `src/vector/vecTable.cpp`, `src/vector/vector_meta.h`
* **Vector Indexing**: `src/vector/semantic/vector_index.h`, `src/vector/semantic/vector_index.cpp`
* **Temporal Bridge**: `src/vector/semantic/candidate_bridge.h`, `src/query/temporal_semantic.h`, `src/query/temporal_semantic.cpp`

---

## Vector Storage: `vecTable`

Vector embeddings are persisted in a dedicated binary companion file named `data/<table_name>/<table_name>.vec`:

```cpp
struct VecRecord {
    uint32_t id = 0;             // Sequential vector ID
    std::string pk;              // Primary key of associated data row
    uint64_t timestamp = 0;      // Timestamp matching data record
    float embedding[VEC_DIM];    // 384 IEEE 754 floats (1,536 bytes)
};
```

### File Header (`VectorMeta`)
The `.vec` file begins with a metadata block storing:
* `metadataSize` (`uint32_t`)
* `name` (`char[30]`)
* `pkSize` (`uint32_t`)
* `recordCount` (`uint32_t`)
* `payloadSize` (`uint32_t` = `sizeof(VecRecord)`)

---

## In-Memory Index: `VectorIndex`

To rapidly identify vectors belonging to candidate records without linear disk scans, MemoraDB maintains an in-memory `VectorIndex`:

```cpp
class VectorIndex {
private:
    vecTable* table = nullptr;
    std::unordered_map<std::string, uint32_t> index; // Maps "pk:timestamp" -> vector ID

    std::string makeKey(const std::string& pk, uint64_t timestamp) const;
public:
    void buildIndex(vecTable& vt);
    uint32_t findId(const std::string& pk, uint64_t timestamp) const;
    std::vector<SearchResult> semanticSearch(
        const std::vector<VCandidate>& candidates,
        const float (&queryEmbedding)[VEC_DIM],
        int32_t k
    );
};
```

When a table is mounted, `VectorIndex::buildIndex` reads all vector records and populates the hash table with composite keys (`pk + ":" + std::to_string(timestamp)`).

---

## Cosine Similarity & Top-k Ranking

### Metric Calculation
Similarity is computed using 384-dimensional cosine similarity:

```text
                       u · v                  Σ (u_i * v_i)
similarity(u, v) = ───────────  =  ───────────────────────────────────
                    ||u|| ||v||     sqrt(Σ (u_i)²) * sqrt(Σ (v_i)²)
```

### Min-Heap Search Algorithm
Instead of sorting all candidate records, `VectorIndex::semanticSearch` maintains a bounded min-heap of size `k`:

```cpp
std::vector<SearchResult> VectorIndex::semanticSearch(
    const std::vector<VCandidate>& candidates,
    const float (&queryEmbedding)[VEC_DIM],
    int32_t k
) {
    if (k <= 0) return {};
    auto compare = [](const SearchResult& a, const SearchResult& b) {
        return a.score > b.score; // Min-heap: lowest score on top
    };

    std::priority_queue<SearchResult, std::vector<SearchResult>, decltype(compare)> heap(compare);
    for (const auto& candidate : candidates) {
        uint32_t id = findId(candidate.pk, candidate.timestamp);
        if (id == UINT32_MAX) continue;
        VecRecord record = table->readRecord(id);
        float score = cosineSimilarity(queryEmbedding, record.embedding);
        SearchResult result{candidate.pk, candidate.timestamp, score};
        if (heap.size() < k) {
            heap.push(result);
        } else if (score > heap.top().score) {
            heap.pop();
            heap.push(result);
        }
    }
    std::vector<SearchResult> results;
    while (!heap.empty()) {
        results.push_back(heap.top());
        heap.pop();
    }
    std::reverse(results.begin(), results.end());
    return results;
}
```

* **Complexity**: `O(N log k)` time, where `N` is the candidate count and `k` is the `LIMIT`.
* **Memory**: `O(k)` auxiliary memory.
