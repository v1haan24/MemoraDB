# Architecture Overview

This section presents the internal architecture of MemoraDB, detailing how its subsystems—storage, indexing, query execution, temporal versioning, and neural vector inference—collaborate to provide a unified DBMS.

---

## Subsystem Breakdown

MemoraDB is organized into six core architectural subsystems:

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        Interactive REPL / CLI                          │
│                (Terminal management, ANSI truecolor)                   │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                         SQL Compiler Pipeline                          │
│      ┌─────────────────┐       ┌─────────────────┐                     │
│      │      Lexer      │  ──►  │ Recursive-      │  ──► AST Statement │
│      │ (Tokenization)  │       │ Descent Parser  │      Variant        │
│      └─────────────────┘       └─────────────────┘                     │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                        Query Execution Engine                          │
│               (Statement dispatch, result construction)                │
└─────────┬─────────────────────────┼──────────────────────────┬─────────┘
          │                         │                          │
          ▼                         ▼                          ▼
┌──────────────────┐      ┌──────────────────┐       ┌──────────────────┐
│ Storage Engine   │      │ Temporal Engine  │       │ Semantic Engine  │
│ • data.db        │      │ • HistoryIndex   │       │ • ONNX Runtime   │
│ • Fixed Payload  │      │ • Binary Search  │       │ • MiniLM Embedder│
│ • Crash Recovery │      │ • Diff / Compare │       │ • vecTable       │
│ • Compaction     │      │ • Rollback       │       │ • VectorIndex    │
└──────────────────┘      └──────────────────┘       └──────────────────┘
```

### 1. The Interactive REPL (`src/cli/`)
The REPL manages user interaction, handles line editing and terminal capabilities, initializes terminal colors, processes administrative meta-commands (`.help`, `.tokens`, `.history`), and coordinates the query execution lifecycle.

### 2. Lexical Analyzer (`src/lexer/`)
The lexer transforms raw SQL strings into a stream of typed `Token` structures. It classifies keywords, identifiers, numeric literals (with unary minus support), string literals, comparison operators, and structural symbols.

### 3. Parser & AST (`src/parser/`)
The recursive-descent parser consumes token streams and constructs strongly-typed Abstract Syntax Tree (AST) nodes. MemoraDB uses a `std::variant<...>` union encompassing all 13 supported statement types (`Statement`).

### 4. Query Executor (`src/engine/` & `src/query/`)
The executor receives an AST `Statement` and dispatches it to dedicated runners:
* **DDL Runner**: Table creation, dropping, and schema inspection.
* **DML Runner**: Positional inserts, condition-based updates, and soft deletes.
* **Query Runner**: Relational filtering (`WHERE`), projection, ordering (`ORDER BY`), and pagination (`LIMIT`).
* **Temporal Runner**: Snapshot generation, version histories, pairwise diffs (`EVOLUTION`), and rollbacks.

### 5. Append-Only Storage Engine (`src/storage/`)
Manages disk persistence inside `data/<table_name>/data.db`. It handles fixed-size binary schema metadata, 9-byte record headers (`timestamp` + `deleted`), and row payloads. It features automated crash recovery and atomic compaction.

### 6. Temporal History Index (`src/index/` & `src/temporal/`)
Maintains an in-memory chronological map of primary keys to record versions (`RecordVersion{timestamp, offset}`). Point-in-time queries use binary search (`latestBefore`) to locate active records in `O(log V)` time.

### 7. Neural Semantic Vector Engine (`src/vector/`)
Integrates Microsoft's ONNX Runtime and the `all-MiniLM-L6-v2` transformer model. Text columns declared `SEMANTIC` are automatically embedded into 384-dimensional dense vectors stored in `data/<table_name>/<table_name>.vec` and searched via cosine similarity.

---

## Subsystem Navigation

* [Query Pipeline](query-pipeline.md) — End-to-end trace from SQL input to result output.
* [Lexer Internals](lexer.md) — Token definitions, scanning rules, and symbol tables.
* [Parser & AST](parser.md) — Grammar productions, AST structures, and error handling.
* [Executor](executor.md) — Statement dispatch, execution logic, and result models.
* [Storage Engine](storage-engine.md) — Binary file layout, record headers, and crash recovery.
* [Temporal Engine](temporal-engine.md) — HistoryIndex implementation, binary search, and diffing algorithms.
* [Semantic Engine](semantic-engine.md) — ONNX Runtime session, vector storage, and top-k min-heap ranking.
