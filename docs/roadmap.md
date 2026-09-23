# Roadmap

This page outlines potential architectural directions and future areas of development for MemoraDB.

---

## Current Status

MemoraDB is currently in an active experimental stage, featuring:
* A working append-only binary storage engine with crash recovery.
* An in-memory temporal index supporting point-in-time snapshots, interval queries, pairwise diffing, and non-destructive rollbacks.
* An embedded ONNX Runtime pipeline running `all-MiniLM-L6-v2` for 384-dimensional vector search.
* An interactive terminal REPL with TrueColor gradients and token inspection.

---

## Areas for Future Development

The following architectural enhancements represent natural extensions to the existing codebase:

### 1. Automated Test Suite
* Introduce an automated unit and integration test framework (e.g. GoogleTest or Catch2).
* Automate regression testing for the lexer, recursive-descent parser, binary storage layout, and temporal algorithms in CI.

### 2. SQL Parser Enhancements
* **Compound WHERE Conditions**: Extend `parseCondition()` to support boolean operators (`AND`, `OR`, `NOT`) with precedence parsing.
* **Secondary Projection Expressions**: Support aliasing (`AS`) and basic scalar expressions in the select list.
* **Named INSERT Columns**: Allow specifying a subset of columns in `INSERT INTO <table> (<cols>) VALUES (...)`.

### 3. Storage & Indexing
* **Secondary B-Tree Indexes**: Implement on-disk or in-memory B-Trees for non-primary-key columns to eliminate linear scans in `WHERE` clauses.
* **Persistent History Index**: Persist the `HistoryIndex` to an index file to eliminate startup recovery scan latency for large databases.
* **Configurable Embedding Models**: Allow specifying alternative ONNX models or custom vector dimensions at table creation time.

### 4. Concurrency & Networking
* **Client-Server Architecture**: Implement a TCP socket server and wire protocol to allow remote client connections to a running MemoraDB daemon.
* **Read-Write Locking**: Introduce reader-writer locks (`std::shared_mutex`) to support concurrent read queries while serializing appends.

