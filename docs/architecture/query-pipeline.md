# The Query Pipeline

This page traces the complete query lifecycle in MemoraDB, following statements from user input through lexing, parsing, execution, and disk I/O.

---

## 1. Trace: Relational `SELECT`

```sql
SELECT id, name FROM students WHERE gpa >= 3.5 ORDER BY gpa DESC LIMIT 5;
```

```text
User Input: "SELECT id, name FROM students WHERE gpa >= 3.5 ORDER BY gpa DESC LIMIT 5;"
    │
    ▼
Lexer: Token stream [SELECT, IDENT(id), COMMA, IDENT(name), FROM, IDENT(students), 
                     WHERE, IDENT(gpa), GREATER_EQUAL, FLOAT_LIT(3.5), 
                     ORDER, BY, IDENT(gpa), DESC, LIMIT, INT_LIT(5), SEMICOLON]
    │
    ▼
Parser: Constructs SelectStmt {
    selectAll: false,
    columns: ["id", "name"],
    tableName: "students",
    temporalMode: NONE,
    where: Condition { column: "gpa", op: GE, value: 3.5 },
    orderByColumn: "gpa",
    orderDescending: true,
    limit: 5
}
    │
    ▼
Executor (`run(const SelectStmt&)`):
    1. Table Lookup: Fetch `Table*` for "students" from Catalog.
    2. Candidate Generation: `table.scanLatest()` fetches current active rows.
    3. WHERE Filtering: `where(rows, meta, clause)` retains rows where gpa >= 3.5.
    4. Sorting: `sortRecords(rows, meta, col, ascending=false)` sorts by float value.
    5. Pagination: `limitRecords(rows, 5)` truncates to 5 rows.
    6. Projection: `project(rows, cols)` extracts only "id" and "name" columns.
    │
    ▼
Output: Formatted table printed to REPL terminal.
```

---

## 2. Trace: `INSERT INTO`

```sql
INSERT INTO notes VALUES (1, "Deployment Guide", "Ensure cluster health prior to upgrade.");
```

```text
User Input: "INSERT INTO notes VALUES (...);"
    │
    ▼
Lexer & Parser: Constructs InsertStmt {
    tableName: "notes",
    values: [Value(1), Value("Deployment Guide"), Value("Ensure cluster...")]
}
    │
    ▼
Executor (`run(const InsertStmt&)`):
    1. Validate Column Count: Check values.size() == meta.columnCount.
    2. Check Primary Key: HistoryIndex::contains("1") ensures uniqueness.
    3. Generate Embedding:
       - Detects column "body" has `isSemantic = 1`.
       - Calls embedder: runs ONNX model in-process to produce float[384].
    4. Append to Storage (`table->insert`):
       - Seek to end of data.db.
       - Write header: [timestamp = now(), deleted = 0].
       - Write row payload (fixed binary layout).
       - HistoryIndex::addVersion("1", {timestamp, file_offset}).
    5. Append to Vector Storage:
       - Appends [id, pk="1", timestamp, embedding] to notes.vec.
    │
    ▼
Output: "1 row inserted"
```

---

## 3. Trace: `UPDATE`

```sql
UPDATE notes SET body = "Updated guide with rollback steps." WHERE id = 1;
```

```text
User Input: "UPDATE notes SET body = '...' WHERE id = 1;"
    │
    ▼
Lexer & Parser: Constructs UpdateStmt {
    tableName: "notes",
    assignments: [Assignment("body", "Updated guide...")],
    where: Condition("id", EQ, 1)
}
    │
    ▼
Executor (`run(const UpdateStmt&)`):
    1. Verify Primary Key Protection: Ensures no assignment targets the PK.
    2. Identify Targets: Scans active rows and matches `id = 1`.
    3. Construct New Row: Copies existing fields, modifies "body".
    4. Re-embed Semantic Columns: Generates new float[384] embedding.
    5. Append New Version:
       - Writes new record to end of data.db (prior version remains untouched!).
       - HistoryIndex registers the new {timestamp, offset} for pk "1".
       - Appends new embedding to notes.vec.
    │
    ▼
Output: "1 row(s) updated"
```

---

## 4. Trace: Temporal Query (`AS OF`)

```sql
SELECT * FROM notes AS OF 2026-09-20 WHERE id = 1;
```

```text
User Input: "SELECT * FROM notes AS OF 2026-09-20 WHERE id = 1;"
    │
    ▼
Executor (`run(const SelectStmt&)`):
    1. Resolve Target Time: dayEndMs("2026-09-20") -> 1726876799999 ms.
    2. Generate Candidates: Calls table.snapshot(1726876799999).
       - For each PK, calls HistoryIndex::latestBefore(pk, targetMs).
       - Binary searches version list for newest record where ts <= targetMs.
       - If deleted == 0, reads payload from disk at version.offset.
    3. Filter: Evaluates WHERE id = 1.
    4. Output: Displays the exact row version active as of that past date!
```

---

## 5. Trace: Semantic Search (`SIMILAR TO`)

```sql
SELECT * FROM notes WHERE body SIMILAR TO "container orchestrator" LIMIT 3;
```

```text
User Input: "SELECT * FROM notes WHERE body SIMILAR TO '...' LIMIT 3;"
    │
    ▼
Executor:
    1. Detects `SIMILAR_TO` operator on SEMANTIC column.
    2. Routes to `temporalSemanticSearch(...)`.
    3. Embeds Query Text: ONNX model generates float queryEmbedding[384].
    4. Gathers Candidate Keys: scanLatestKeys() returns active {pk, ts} pairs.
    5. Cosine Similarity & Min-Heap:
       - For each candidate, reads embedding from notes.vec.
       - Computes dot product and Euclidean norm.
       - Retains top 3 highest scores in min-heap.
    6. Decorate: exactVersion() retrieves semantic text values from data.db.
    │
    ▼
Output: Ranked search results table with similarity scores.
```
