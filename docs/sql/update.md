# UPDATE

## Description

The `UPDATE` statement modifies column values for existing rows. In MemoraDB's append-only architecture, an update does **not** overwrite the existing bytes on disk; instead, it creates and appends a **new version** of each target row with an updated timestamp.

---

## Syntax

*(See the [Syntax Notation Guide](../reference/grammar.md#syntax-notation-guide) for notation rules).*

### Basic syntax

```sql
UPDATE <table> SET <col> = <val> WHERE <condition>;
```

### Full syntax

```sql
UPDATE <table> SET <col1> = <v1>[, <col2> = <v2> ...] [WHERE <condition>];
```

---

## Parameters

| Parameter | Description |
|---|---|
| `<table>` | The target table name. |
| `SET <col> = <v>` | One or more column assignments separated by commas. |
| `WHERE <condition>` | Optional filter condition. If omitted, **every** active row in the table is updated. |

> [!WARNING]
> **Primary Key Immutability**: You **cannot** update the primary key column. Primary keys serve as the immutable anchor for record version chains. To change a primary key, you must `DELETE` the record and `INSERT` a new record.

---

## Examples

### 1. Update Specific Row with WHERE (`students`)

```sql
UPDATE students SET gpa = 3.95 WHERE id = 1;
```

### 2. Update Multiple Columns (`students`)

```sql
UPDATE students SET gpa = 4.00, is_enrolled = true WHERE id = 1;
```

### 3. Update Semantic Text Column (`documents`)

```sql
UPDATE documents 
SET content = "All microservices deployed cleanly to production cluster." 
WHERE id = 1;
```

---

## Expected Output

```text
1 row(s) updated
```

---

## How It Works

1. **Target Identification**: The engine scans the latest active rows (`table->scanLatest()`) and filters them against the `WHERE` clause (if provided).
2. **Assignment Validation**:
   * Validates that all target columns exist in the schema.
   * Verifies that no assignment attempts to alter the primary key column.
   * Validates each new value against the column's data type and size constraints.
3. **Append New Version**:
   * For each matched record, a new `Row` is constructed with the modified values.
   * If the table contains a `SEMANTIC` column, a new 384-dimensional embedding is generated via ONNX Runtime.
   * The new version is appended to the end of `data.db` (and `.vec`).
   * The in-memory `HistoryIndex` registers the new `{timestamp, file_offset}` for that primary key.

---

## Errors & Edge Cases

| Error Condition | Error Message |
|---|---|
| Updating Primary Key | `Cannot UPDATE the primary key column '<col>' (delete and re-insert instead)` |
| Unknown Column in SET | `Unknown column '<col>' on table '<table>'` |
| Unknown Column in WHERE | `Unknown column '<col>' on table '<table>'` |
| Type Mismatch in Value | Validation failure during assignment parsing. |

---

## Related Commands

* [`INSERT`](insert.md) — Insert new rows.
* [`DELETE`](delete.md) — Soft-delete rows.
* [`HISTORY`](../temporal/history.md) — View the timeline of updates for a row.
* [`EVOLUTION`](../temporal/evolution.md) — Track step-by-step column diffs across updates.
