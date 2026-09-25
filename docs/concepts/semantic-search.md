# Semantic Search Concepts

This page explains the principles of semantic vector search, high-dimensional embedding spaces, and how MemoraDB integrates deep neural models directly into query execution.

---

## Keyword Search vs. Semantic Search

Traditional database queries rely on exact literal matching or pattern matching (e.g., `WHERE col = 'value'` or `WHERE col LIKE '%pattern%'`):

```text
Query: "automobile repairs"

Keyword Matching:
  ❌ "Vehicle maintenance guide"       -> No match (no overlapping words)
  ❌ "Car engine troubleshooting"     -> No match
  ✓ "Automobile repairs checklist"    -> Match
```

**Semantic Search** transforms unstructured text into dense numerical vectors that represent the *meaning* and *context* of the language. In a semantic search system:

```text
Query: "automobile repairs"

Semantic Vector Matching:
  ✓ "Vehicle maintenance guide"       -> High Similarity (91%)
  ✓ "Car engine troubleshooting"     -> High Similarity (88%)
  ✓ "Automobile repairs checklist"    -> High Similarity (94%)
  ❌ "Baking chocolate chip cookies"   -> Low Similarity (12%)
```

---

## High-Dimensional Vector Embeddings

An **embedding** is a vector of real numbers in a continuous vector space (continuous 384-dimensional vector space). Words and sentences with similar meanings are projected to points that lie close to one another in this space.

MemoraDB uses the **`all-MiniLM-L6-v2`** sentence-transformer model:
* **Dimensionality (`D`)**: 384 floating-point numbers (`VEC_DIM = 384`).
* **Input**: Arbitrary text string up to model sequence limits.
* **Output**: A normalized 384-dimensional vector (`float[384]`).

---

## Cosine Similarity

To determine how similar two pieces of text are, MemoraDB calculates the **cosine similarity** between their respective embedding vectors `u` and `v`:

```text
                       u · v                  Σ (u_i * v_i)
similarity(u, v) = ───────────  =  ───────────────────────────────────
                    ||u|| ||v||     sqrt(Σ (u_i)²) * sqrt(Σ (v_i)²)
```

### Properties of Cosine Similarity:
* **Range**: `[-1.0, 1.0]` (in practice, sentence embeddings typically fall between `0.0` and `1.0`).
* **Angle Metric**: It measures the cosine of the angle between two vectors, making it invariant to the absolute magnitude (length) of the vectors.
* **Interpretation**:
  * Near `1.0`: Nearly identical semantic meaning.
  * `0.7` – `0.9`: Strongly related topics or paraphrases.
  * Below `0.4`: Unrelated or loosely related concepts.

---

## How MemoraDB Implements Vector Search

Unlike architectures that require a separate vector database (like Pinecone, Milvus, or Qdrant) alongside a relational database, MemoraDB embeds vector storage and inference **in-process**:

1. **Schema Declaration**: Any `STRING` column marked with the `SEMANTIC` modifier is designated for vector embedding.
2. **On-Write Embedding**: When a row is inserted or updated, MemoraDB extracts the text from all `SEMANTIC` columns, passes it through the in-process ONNX Runtime session, and obtains a 384-dimensional vector.
3. **Dedicated Vector Storage (`vecTable`)**: Embeddings are written to a companion binary file `data/<table_name>/<table_name>.vec`. Each vector record stores:
   * `id`: 32-bit vector identifier.
   * `pk`: Primary key string.
   * `timestamp`: Creation timestamp matching the data row.
   * `embedding`: 384 IEEE 754 float values (1,536 bytes).
4. **Ranking via Priority Queue**: During a `SIMILAR TO` query, candidate rows are evaluated against the query vector. A min-heap (`std::priority_queue`) retains the top-k highest scoring records.

---

## Temporal-Semantic Synergy

A unique capability of MemoraDB is combining temporal filters with semantic search:

```sql
SELECT * FROM notes 
BETWEEN 2026-01-01 AND 2026-06-01 
WHERE content SIMILAR TO "quarterly financial report" 
LIMIT 5;
```

In this query:
1. The **Temporal Engine** identifies all candidate records whose version timestamps fall between the two dates.
2. The **Semantic Engine** evaluates cosine similarities only across those specific temporal candidates.
3. The result is a ranked list of historically accurate semantic matches!
