# Current Engine Limitations

MemoraDB is designed as an educational, experimental database management system. To maintain clarity, educational value, and architectural coherence, several trade-offs and deliberate constraints are present in the current implementation.

---

## 1. SQL Dialect Limitations

* **Single Condition in `WHERE`**: The parser supports only a single condition in the `WHERE` clause. Compound boolean expressions with `AND`, `OR`, or `NOT` are not supported.
* **No `JOIN` Operations**: MemoraDB operates on single tables. Multi-table queries, cross-joins, and relational joins are not implemented.
* **No Aggregate Functions**: Expressions such as `COUNT(*)`, `SUM()`, `AVG()`, `MIN()`, or `MAX()` are not supported in SQL statements.
* **No `GROUP BY` or `HAVING`**: Data grouping and aggregated filtering are not supported.
* **No Subqueries**: Nested queries or sub-selects are not supported.
* **Positional `INSERT` Only**: An `INSERT` statement must specify values for all columns in definition order. Partial column inserts (`INSERT INTO t (col1) VALUES (...)`) are not supported.

---

## 2. Data Types & Schema Constraints

* **Fixed-Width Strings**: Every `STRING` column must have an explicit byte length declared during `CREATE TABLE` (e.g., `STRING(50)`). Dynamic or unbounded strings are not supported.
* **No Date/Time Column Type**: While MemoraDB uses high-resolution timestamps internally for temporal versioning, there is no user-facing `DATE` or `DATETIME` column type for application data.
* **Identifier Length Limits**: Table names and column names are restricted to a maximum of 29 characters (`tns = 30` and `cns = 30` including the null terminator).
* **Strict Primary Key Requirement**: Exactly one column per table must be designated `PRIMARY KEY`. Composite primary keys and primary-key-less tables are not supported.
* **Immutable Primary Keys**: Primary key values cannot be altered via `UPDATE` (they must be deleted and re-inserted).

---

## 3. Concurrency & Transactions

* **Single-Process Embedded Model**: MemoraDB is an embedded database operated via an interactive REPL or linked library. It does not run as a background client-server daemon and does not manage multi-client network connections.
* **No Multi-Threading Lock Manager**: The engine assumes single-threaded access per table. Concurrent concurrent readers and writers across multiple threads are not coordinated via a lock manager.
* **No Multi-Statement ACID Transactions**: MemoraDB does not implement `BEGIN TRANSACTION`, `COMMIT`, or write-ahead logging (WAL). While individual statements are atomic and append-only, grouping multiple statements into an ACID transaction is not supported.
* **Rollback Semantics**: The `ROLLBACK` statement in MemoraDB is a temporal version restoration operation (it appends past records as new versions), not an uncommitted transaction rollback.

---

## 4. Indexing & Storage Engine

* **No Secondary B-Trees**: The only indexes in MemoraDB are the in-memory `HistoryIndex` (which indexes primary keys) and the `VectorIndex` (which indexes vector embeddings). Queries filtering on non-primary-key columns perform a full table scan over candidate records.
* **In-Memory History Index**: The `HistoryIndex` is reconstructed in memory on startup by scanning `data.db`. Tables with millions of versions will experience startup reconstruction latency proportional to the version count.
* **Unbounded Growth Without Compaction**: Because writes are strictly append-only, disk usage increases with every `UPDATE` and `DELETE` until an explicit `COMPACT TABLE` is executed.

---

## 5. Semantic Search Constraints

* **Fixed Model**: The embedding pipeline is hardcoded to the 384-dimensional `all-MiniLM-L6-v2` architecture (`VEC_DIM = 384`). Custom models or variable-dimension vectors are not supported without re-compiling.
* **CPU Inference**: ONNX inference runs on the host CPU using standard CPU execution providers; GPU acceleration (CUDA, DirectML, CoreML) is not currently configured in `CMakeLists.txt`.
* **String-Only Embeddings**: The `SEMANTIC` modifier is valid only on `STRING` columns.

---

## 6. Platform Support

* **64-Bit Only**: Automatic downloads and memory layouts require 64-bit operating systems (`x86_64` on Windows and Linux; `universal2` on macOS). 32-bit platforms are not supported.
