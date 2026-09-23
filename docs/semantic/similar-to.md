# SIMILAR TO Queries

## Description

The `SIMILAR TO` operator performs semantic similarity search on text columns declared with the `SEMANTIC` modifier. It converts the query text into a dense vector embedding, compares it against stored row vectors using cosine similarity, and returns the highest-scoring records ranked from most relevant to least relevant.

---

## Syntax

*(See the [Syntax Notation Guide](../reference/grammar.md#syntax-notation-guide) for notation rules).*

### Basic syntax

```sql
SELECT * FROM <table> WHERE <semantic_col> SIMILAR TO "<query text>";
```

### Full syntax

```sql
SELECT * | <columns> 
FROM <table> 
[ <temporal-clause> ]
WHERE <semantic_col> SIMILAR TO "<query text>"
[ <temporal-clause> ]
[ LIMIT <n> ];
```

> [!NOTE]
> In `SELECT` statements, the temporal clause and `WHERE ... SIMILAR TO` can be written in **either order**:
> ```sql
> SELECT * FROM documents AS OF 2026-09-01 WHERE content SIMILAR TO "deployment error" LIMIT 5;
> SELECT * FROM documents WHERE content SIMILAR TO "deployment error" AS OF 2026-09-01 LIMIT 5;
> ```

---

## Parameters

| Parameter | Description |
|---|---|
| `<semantic_col>` | The name of a column declared `SEMANTIC` in `CREATE TABLE`. |
| `"<query text>"` | String literal containing the natural-language search query. |
| `LIMIT <n>` | Restricts results to the top `<n>` matches. If omitted, **defaults to 10**. |

---

## Examples (`documents`)

### 1. Basic Semantic Search

```sql
SELECT * FROM documents WHERE content SIMILAR TO "database crash recovery" LIMIT 3;
```

---

## Output Structure

The output displays candidate records ranked in descending order of similarity score:
1. `pk`: Primary key of the matching record.
2. `timestamp`: Version timestamp of the record.
3. `score`: Runtime-computed cosine similarity score formatted to 4 decimal places.
4. `<semantic_col>`: The text content of the semantic column(s).

```text
--------------------------------------------------------------------------------------------------------------------
pk    timestamp                score    content
--------------------------------------------------------------------------------------------------------------------
1     [Runtime Timestamp]      [Score]  A database stores structured information and implements WAL crash recovery.
4     [Runtime Timestamp]      [Score]  Operating systems manage computer hardware and handle system exceptions.
2     [Runtime Timestamp]      [Score]  The CPU executes instructions and performs arithmetic calculations.
--------------------------------------------------------------------------------------------------------------------
3 result(s)
```

*(Note: Exact similarity scores depend on the vector embedding generated at runtime by the `all-MiniLM-L6-v2` ONNX model).*

---

### 2. Temporal-Semantic Search with `AS OF`

Find documents most similar to "compiler optimization" as the database existed last month:

```sql
SELECT * FROM documents 
AS OF 2026-08-01 
WHERE content SIMILAR TO "compiler optimization" 
LIMIT 5;
```

### 3. Temporal-Semantic Search with `BETWEEN`

Find historical versions from Q1 2026 that discuss "security vulnerabilities":

```sql
SELECT * FROM documents 
BETWEEN 2026-01-01 AND 2026-03-31 
WHERE content SIMILAR TO "security vulnerabilities" 
LIMIT 5;
```

---

## How Scores are Interpreted

* **`1.0000`**: Exact semantic match (identical meaning).
* **High Relevance**: Strong conceptual overlap or close paraphrasing.
* **Moderate Relevance**: Related technical concepts or shared domain context.
* **Low Relevance**: Distant or unrelated topics.

---

## Errors & Edge Cases

| Error Condition | Error Message |
|---|---|
| Non-Semantic Column | `Column '<col>' is not declared SEMANTIC, so SIMILAR TO can't be used on it` |
| No Vector Table Found | `No vector table found for '<table>' -- no semantic index is available` |
| Embedder Not Initialized | `SIMILAR TO needs the query text embedded, but no embedding provider is configured...` |
| Empty Query Text | Model returns standard zero-vector; score defaults to `0.0`. |

---

## Related Commands

* [`SELECT`](../sql/select.md) — Main query documentation.
* [`Embedding Pipeline`](embedding-pipeline.md) — How queries are converted into vectors.
* [`How It Works`](how-it-works.md) — Detailed ranking mechanics.
