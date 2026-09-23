# Repository Structure

This page documents the source code tree of MemoraDB to help developers locate files, understand module boundaries, and navigate the codebase.

---

## Workspace Layout

```text
MemoraDB/
├── CMakeLists.txt                 # Root CMake build configuration
├── mkdocs.yml                     # Documentation website configuration
├── .github/
│   └── workflows/
│       └── deploy-docs.yml        # GitHub Pages documentation deployment
├── docs/                          # Documentation source files
├── test/                          # Automated developer test suite (GoogleTest + CTest)
│   ├── lexer/                     # Lexer unit tests
│   ├── parser/                    # Parser unit tests
│   ├── catalog/                   # Catalog schema & table management tests
│   ├── storage/                   # Storage engine binary tests
│   ├── query/                     # Query execution tests
│   ├── temporal/                  # Temporal operations tests
│   ├── index/                     # HistoryIndex tests
│   ├── vector/                    # Cosine similarity & mock semantic search tests
│   └── integration/               # End-to-end multi-statement lifecycle tests
└── src/                           # Core DBMS source code
    ├── catalog/                   # Database catalog & table lifecycle
    ├── cli/                       # Terminal REPL & UI
    ├── common/                    # Shared constants, metadata, time utilities
    ├── engine/                    # Execution engine & statement runners
    ├── index/                     # In-memory temporal history index
    ├── lexer/                     # Lexical analysis & token definitions
    ├── parser/                    # Recursive-descent parser & AST
    ├── query/                     # Query filtering, sorting, projection
    ├── storage/                   # Binary append-only storage engine
    ├── temporal/                  # Temporal operations (AS OF, BETWEEN, etc.)
    └── vector/                    # ONNX Runtime embedder & vector index
```

---

## Detailed Source Tree Tour

### 1. `src/catalog/`
Manages the mounting, creation, dropping, and inspection of tables.
* `catalog.h` / `catalog.cpp` — Declares and implements `Catalog`. Loads tables from `data/`, calculates column offsets, and writes initial metadata headers.

### 2. `src/cli/`
Implements the user-facing command-line shell.
* `repl.h` / `repl.cpp` — Interactive REPL loop, prompt handling, signal trapping (`SIGINT`), meta-command dispatch (`.help`, `.tokens`, `.history`, `.clear`), and color banner formatting.
* `terminal.h` / `terminal.cpp` — Low-level terminal abstractions, ANSI escape codes, TrueColor detection, cursor and window title manipulation.

### 3. `src/common/`
Shared definitions and utility headers used across all modules.
* `constants.h` — Fixed limits: column name length (`cns = 30`), table name length (`tns = 30`), row header size (`rhsz = 9`), vector dimension (`VEC_DIM = 384`).
* `metadata.h` — Data structures: `DataType`, `ColMeta`, `TableMeta`, `RecordVersion`, `Row`, `Record`, `Difference`, `VCandidate`, `SearchResult`.
* `time_format.h` — High-resolution timestamp formatting (`formatTimestamp(ms)` -> `YYYY-MM-DD HH:MM:SS.mmm`).

### 4. `src/engine/`
Coordinates statement execution and bridges subsystems.
* `executor.h` / `executor.cpp` — Defines `Executor`, `ExecResult`, civil date conversion (`dayStartMs`, `dayEndMs`), and statement routing via `std::visit`.
* `executor_dml.cpp` — Execution logic for `CREATE TABLE`, `DROP TABLE`, `DESCRIBE TABLE`, `SHOW TABLES`, `INSERT`, `UPDATE`, `DELETE`.
* `executor_query.cpp` — Execution logic for `SELECT` (both ordinary relational queries and `SIMILAR TO` semantic searches).
* `executor_temporal.cpp` — Execution logic for `COMPARE`, `EVOLUTION`, `HISTORY`, `ROLLBACK`, and `COMPACT`.

### 5. `src/index/`
In-memory indexing for temporal history.
* `history_index.h` / `history_index.cpp` — `HistoryIndex` mapping primary keys to chronological vectors of `RecordVersion`. Implements binary search in `latestBefore(pk, ts)`.

### 6. `src/lexer/`
Lexical analysis.
* `token.h` — Defines `TokenType` enum and `Token` struct.
* `lexer.h` / `lexer.cpp` — Implements `Lexer`, character scanning, keyword recognition, and literal parsing.

### 7. `src/parser/`
Recursive-descent SQL compiler.
* `ast.h` — AST node definitions and `Statement` variant.
* `parser.h` / `parser_core.cpp` — Parser base class, token navigation, error throwing.
* `parser_statement.cpp` — Top-level statement dispatcher.
* `parser_select.cpp` — Parsing `SELECT` with flexible clause ordering.
* `parser_ddl.cpp` — Parsing `CREATE`, `DROP`, `DESCRIBE`, `SHOW`.
* `parser_dml.cpp` — Parsing `INSERT`, `UPDATE`, `DELETE`.
* `parser_compare.cpp` — Parsing `COMPARE`, `EVOLUTION`, `HISTORY`.
* `parser_rollback.cpp` — Parsing `ROLLBACK` and `COMPACT`.
* `parser_leaf.cpp` — Parsing literals, operators, conditions, and dates with calendar validation.

### 8. `src/query/`
Query pipeline operations.
* `query.h` — Declarations for candidate generation, filtering, and sorting.
* `candidate.cpp` — `generateCandidates` and `generateCandidateKeys` for all temporal modes.
* `where.cpp` — Evaluates relational comparison operators (`=`, `!=`, `<`, `<=`, `>`, `>=`).
* `project.cpp` — Projects record columns based on select list.
* `misc.cpp` — Type-aware sorting (`sortRecords`) and row limiting (`limitRecords`).
* `temporal_semantic.h` / `temporal_semantic.cpp` — Coordinates temporal candidate generation with vector search.
* `vector_compact.h` / `vector_compact.cpp` — Synchronizes relational and vector compaction.

### 9. `src/storage/`
Append-only binary persistence and crash recovery.
* `table.h` / `table.cpp` — `Table` class managing binary file handles and disk operations.
* `insert.cpp` — Appending row records to `data.db`.
* `update.cpp` — Appending updated versions.
* `delete.cpp` — Appending deletion tombstones.
* `read.cpp` — Reading binary payloads from file offsets.
* `recovery.cpp` — Startup state verification and automated file truncation.
* `compact.cpp` — Pruning superseded history and archiving data files.
* `serialization.cpp` — Low-level binary read/write primitives.
* `table_validation.cpp` — Type and capacity validation for row values.
* `display.cpp` — Console printing helpers for rows, records, and diffs.
* `semantic_storage.cpp` — Writing embeddings into companion `.vec` files.

### 10. `src/temporal/`
Temporal database algorithms.
* `as_of.cpp` — Point-in-time record resolution.
* `between.cpp` — Range-based version scanning.
* `compare.cpp` — Net difference calculation across range endpoints.
* `evolution.cpp` — Sequential pairwise difference changelog.
* `history.cpp` — Full version chain retrieval.
* `rollback.cpp` — Non-destructive historical restoration.
* `snapshot.cpp` — Table-wide point-in-time state reconstruction.

### 11. `src/vector/`
Neural embeddings and vector search.
* `minilm_embedder.h` / `minilm_embedder.cpp` — In-process ONNX Runtime session and WordPiece tokenizer.
* `vecTable.h` / `vecTable.cpp` — Persistence manager for `data/<table_name>/<table_name>.vec`.
* `vector_meta.h` / `vector_meta.cpp` — Vector table header serialization.
* `vec_compact.cpp` — Compaction rewrite routines for vector tables.
* `semantic/vector_index.h` / `semantic/vector_index.cpp` — In-memory index and top-k min-heap cosine search.
* `semantic/candidate_bridge.h` / `semantic/candidate_bridge.cpp` — Bridges temporal candidate keys to vector IDs.

---

### 12. `test/`
Automated developer test suite using GoogleTest.
* `CMakeLists.txt` — Declares `memora_tests` target, fetches GoogleTest via `FetchContent`, registers with CTest.
* `test_main.cpp` — Main test runner entry point.
* `test_helper.h` — RAII `TempDirectory` fixture isolating disk writes from the user's workspace.
* `lexer/test_lexer.cpp` — Tokenization, keyword recognition, literals, operators, positions.
* `parser/test_parser.cpp` — AST parsing for DDL, DML, SELECT, temporal, semantic syntax.
* `catalog/test_catalog.cpp` — Schema validation, duplicate table rejection, reload persistence.
* `storage/test_storage.cpp` — Row validation, append-only records, tombstone deletion, record diffs.
* `query/test_executor.cpp` — Execution engine tests (DDL/DML/SELECT, projections, limits, sorting).
* `temporal/test_temporal.cpp` — Temporal engine tests (history, AS OF, BETWEEN, snapshot, rollback).
* `index/test_index.cpp` — HistoryIndex binary search and version tracking.
* `vector/test_vector.cpp` — Cosine similarity calculation and deterministic mock semantic queries.
* `integration/test_integration.cpp` — End-to-end multi-statement workflows across restart lifecycles.

