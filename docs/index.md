# MemoraDB Documentation

Welcome to the official documentation for **MemoraDB**, a custom relational database management system built from scratch in C++17 featuring an append-only binary storage engine, temporal versioning, and in-process semantic vector search powered by ONNX Runtime.

![MemoraDB Terminal](assets/terminal-hero.png)

---

## What is MemoraDB?

MemoraDB is an educational and experimental DBMS designed to explore two modern database frontiers within a single, coherent architecture:

1. **Temporal Data Management**: Records are never destructively overwritten or silently erased. Every `INSERT`, `UPDATE`, and `DELETE` operation appends an immutable version tagged with a high-resolution millisecond timestamp. This enables historical queries, point-in-time snapshots, schema diffing, and timeline rollback.
2. **In-Process Semantic Search**: By embedding Microsoft's ONNX Runtime and the `all-MiniLM-L6-v2` transformer model directly into the query execution engine, MemoraDB can index text columns as dense 384-dimensional vectors and perform cosine similarity search via the `SIMILAR TO` operator without external vector databases.

```sql
-- Create a table with temporal versioning and semantic vector indexing
CREATE TABLE documents (
    id INT PRIMARY KEY,
    title STRING(60),
    content STRING(500) SEMANTIC
);

-- Query records as they existed at a historical timestamp
SELECT * FROM documents AS OF 2026-09-20 WHERE id = 1;

-- Search text conceptually using machine learning embeddings
SELECT * FROM documents WHERE content SIMILAR TO 'database deployment issues' LIMIT 5;
```

---

## Key Features

* **Append-Only Binary Storage Engine**: Table data is stored in a structured binary format (`data/<table_name>/data.db`) consisting of fixed-size metadata, record headers (timestamp, tombstone flag), and column payloads.
* **Zero-Loss Immutability**: Updates and deletes append new versions. Deleted rows receive tombstone records rather than being excised from disk.
* **In-Memory History Index**: Fast `O(log V)` point-in-time version resolution using an in-memory chronological index with binary search (`latestBefore`).
* **Time-Travel Querying**: Query tables using `AS OF <date>`, `SNAPSHOT <date>`, and `BETWEEN <date1> AND <date2>`.
* **State Evolution & Comparison**: Built-in `COMPARE` and `EVOLUTION` statements compute field-by-field diffs across versions over any time range.
* **Non-Destructive Rollback**: Roll back specific rows (`ROLLBACK <table> WHERE ... TO <date>`) or entire tables (`ROLLBACK TABLE <table> TO <date>`) by reading past states and appending them as new versions.
* **History Compaction & Archiving**: `COMPACT TABLE <table> TO <date>` purges superseded history prior to an anchor timestamp while safely archiving the previous data file to `archive/`.
* **Deep Neural Embeddings**: Built-in ONNX Runtime integration with Hugging Face `all-MiniLM-L6-v2` generates 384-dimensional vector embeddings on the fly for columns declared `SEMANTIC`.
* **Integrated Vector Index**: Vector embeddings are persisted in dedicated `.vec` tables and indexed with top-k min-heap priority queues using cosine similarity.
* **Combined Temporal-Semantic Queries**: Seamlessly filter by time and semantic similarity in a single query (e.g., finding past documents similar to a query text as of last month).
* **Interactive Terminal REPL**: Features syntax-highlighted token inspection (`.tokens`), command history (`.history`), built-in command assistance (`.help`), and crash recovery.

---

## Why MemoraDB?

Traditional relational databases treat current state as the primary reality, relegating history to auxiliary audit logs or write-ahead logs that are difficult to query directly. Conversely, dedicated vector databases excel at embedding search but lack relational schema constraints and temporal lineage.

MemoraDB bridges these concepts:
* **Educational Clarity**: Built from the ground up without third-party DBMS dependencies to show how lexers, parsers, executors, storage managers, temporal indexes, and embedding engines work together.
* **Unified Query Model**: Express temporal constraints and semantic similarity within standard SQL-like statements.
* **Self-Contained Architecture**: Embedded engine with in-process vector inference; no external daemon, socket server, or external Python runtime required at execution time.

---

## Quick Start

### 1. Launch the Shell

Run the compiled executable:

```powershell
.\build\memora.exe
```

You will see the MemoraDB banner and system diagnostic indicators:

```text
  ●  temporal engine ready              ●  append-only storage ready
  ●  semantic vector search ready        ●  embedding model ready
```

### 2. Create a Table and Insert Records

```sql
CREATE TABLE documents (
    id INT PRIMARY KEY,
    title STRING(60),
    content STRING(300) SEMANTIC
);

INSERT INTO documents VALUES (1, "Databases", "Relational databases use tables, rows, and B-tree indexes.");
INSERT INTO documents VALUES (2, "DevOps", "Kubernetes automates container deployment, scaling, and operations.");
INSERT INTO documents VALUES (3, "Artificial Intelligence", "Large language models generate text based on neural transformer architectures.");
```

### 3. Run a Semantic Search

```sql
SELECT * FROM documents WHERE content SIMILAR TO "orchestrating docker containers" LIMIT 3;
```

### 4. Update and Inspect History

```sql
UPDATE documents SET content = "Relational databases support SQL queries and transactional ACID guarantees." WHERE id = 1;

HISTORY documents WHERE id = 1;
```

---

## How It Works

MemoraDB executes statements through a modular pipeline:

```text
User Input / SQL
       │
       ▼
 ┌───────────┐
 │   Lexer   │  Tokenizes SQL into keywords, identifiers, literals, operators
 └─────┬─────┘
       │
       ▼
 ┌───────────┐
 │  Parser   │  Recursive-descent AST construction with syntax validation
 └─────┬─────┘
       │
       ▼
 ┌───────────┐
 │ Executor  │  Coordinates Catalog, Table Engine, Vector Engine, & Embedder
 └─────┬─────┘
       │
       ├─────────────────────────┬─────────────────────────┐
       ▼                         ▼                         ▼
┌──────────────┐          ┌──────────────┐          ┌──────────────┐
│ Storage &    │          │   Temporal   │          │ Vector Table │
│ HistoryIndex │          │ Diff Engine  │          │ & ONNX Model │
└──────────────┘          └──────────────┘          └──────────────┘
```

For detailed architectural breakdowns, see the [Architecture Overview](architecture/overview.md).

---

## Documentation Guide

* **[Getting Started](getting-started/quick-start.md)**: System prerequisites, compilation instructions, and first steps.
* **[Core Concepts](concepts/overview.md)**: Relational fundamentals, temporal data theory, and semantic vector mechanics.
* **[SQL Reference](sql/overview.md)**: Syntax, parameters, examples, and rules for all supported statements.
* **[Temporal Database](temporal/overview.md)**: In-depth guide to `AS OF`, `BETWEEN`, `HISTORY`, `EVOLUTION`, `COMPARE`, `ROLLBACK`, and `COMPACT`.
* **[Semantic Search](semantic/overview.md)**: Vector indexing, the ONNX pipeline, cosine similarity, and `SIMILAR TO` usage.
* **[Architecture](architecture/overview.md)**: Code-level tour of the storage format, memory layout, lexer, parser, and executor.
* **[Tutorials](tutorials/student-database.md)**: Step-by-step walkthroughs solving concrete data modeling scenarios.
* **[Developer Guide](development/building-from-source.md)**: Source tree explanation, build scripts, dependencies, and testing.
* **[Troubleshooting](troubleshooting/common-issues.md)**: Solutions for compiler configurations, model paths, and runtime issues.
* **[Limitations](limitations.md)**: Explicit boundaries of the current SQL dialect and engine.
