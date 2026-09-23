# INSERT

## Description

The `INSERT INTO` statement appends a new record to a table. Because MemoraDB is an append-only DBMS, every insert writes an immutable record stamped with the current system millisecond timestamp and creates the initial version in that row's version chain.

---

## Syntax

*(See the [Syntax Notation Guide](../reference/grammar.md#syntax-notation-guide) for notation rules).*

### Basic syntax

```sql
INSERT INTO <table> VALUES (<v1>, <v2>);
```

### Full syntax

```sql
INSERT INTO <table> VALUES (<v1>, <v2>, ...);
```

---

## Parameters

| Parameter | Description |
|---|---|
| `<table>` | The target table name. |
| `<v1>, <v2>, ...` | Positional values corresponding to every column in the table in the exact order declared during `CREATE TABLE`. |

> [!IMPORTANT]
> Values are strictly **positional**. You cannot specify a custom column subset or reorder columns in `INSERT`. All column values must be provided.

---

## Examples

### 1. Insert into Relational Table (`students`)

```sql
INSERT INTO students VALUES (1, "Alice Chen", 3.85, true);
```

### 2. Insert into Semantic Table (`documents`)

```sql
INSERT INTO documents VALUES (1, "Architecture Overview", "MemoraDB is an append-only DBMS with temporal and semantic search.");
```

---

## Expected Output

```text
1 row inserted
```

---

## How It Works

1. **Column Count Check**: Verifies that the number of values in the statement matches `meta.columnCount`.
2. **Type Validation**: Validates that each literal conforms to the expected column data type (`INT`, `FLOAT`, `STRING`, `BOOL`).
3. **Primary Key Uniqueness**: Extracts the primary key value. If `HistoryIndex::contains(pk)` returns true, the insert is rejected (primary keys must be unique).
4. **Append Record**:
   * Seeks to the end of `data/<table_name>/data.db`.
   * Writes the 9-byte header: `timestamp = current_time_ms()` and `deleted = 0`.
   * Writes the fixed-size row payload.
5. **Update History Index**: Adds `{timestamp, file_offset}` to the table's in-memory `HistoryIndex`.
6. **Generate Semantic Vector**: If the table contains any `SEMANTIC` column, the text from all semantic columns is concatenated and passed to the ONNX `MiniLmEmbedder`. The resulting 384-dimensional vector is appended to `data/<table_name>/<table_name>.vec`.

---

## Errors & Edge Cases

| Error Condition | Error Message |
|---|---|
| Value Count Mismatch | `Table '<table>' has X columns but Y values were given` |
| Primary Key Already Exists | `Primary key '<pk>' already exists.` |
| Value Exceeds String Size | Rejection during row validation if string exceeds declared byte capacity. |
| Embedding Generation Failure | `Failed to create ONNX embedding` |

---

## Related Commands

* [`SELECT`](select.md) — Query inserted rows.
* [`UPDATE`](update.md) — Modify existing rows by appending new versions.
* [`DELETE`](delete.md) — Soft-delete rows via tombstones.
