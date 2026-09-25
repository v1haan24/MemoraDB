# AS OF / SNAPSHOT

## Description

The `AS OF` and `SNAPSHOT` clauses reconstruct the state of a table as of a specific past date or instant. Both keywords are syntactically and semantically identical in MemoraDB.

---

## Syntax

*(See the [Syntax Notation Guide](../reference/grammar.md#syntax-notation-guide) for notation rules).*

### Basic syntax

```sql
SELECT * FROM <table> AS OF <date>;
```

### Full syntax

```sql
SELECT * | <columns> FROM <table> (AS OF | SNAPSHOT) <date> [WHERE <condition>];
```

> [!NOTE]
> **Flexible Clause Ordering**: In `SELECT` statements, the temporal clause and `WHERE` clause can be written in either order:
> ```sql
> SELECT * FROM employees AS OF 2026-09-20 WHERE id = 1;
> SELECT * FROM employees WHERE id = 1 AS OF 2026-09-20;
> ```

---

## Date-Only vs. Timestamp Semantics

A critical detail in MemoraDB's temporal engine is how date literals are interpreted:

### 1. Date-Only Literals (`YYYY-MM-DD`)
When you provide a bare date without a time portion (e.g., `2026-09-20`), the engine interprets this as the **end of that whole day** (`23:59:59.999`).
* This ensures that any records inserted or updated at any point during that calendar day are included in the query result.

### 2. Explicit Timestamp Literals (`YYYY-MM-DD HH[:MM[:SS[.mmm]]]`)
When you provide an explicit time (e.g., `2026-09-20 00:00:00.000`), the engine evaluates the snapshot at that **exact millisecond instant**.

```sql
-- Evaluates state at the END of September 20 (23:59:59.999)
SELECT * FROM employees AS OF 2026-09-20;

-- Evaluates state at the EXACT START of September 20 (00:00:00.000)
SELECT * FROM employees AS OF 2026-09-20 00:00:00.000;
```

---

## How It Works

1. **Date Parsing**: The date string is parsed into a 64-bit epoch timestamp in milliseconds (`Executor::dayEndMs`).
2. **Anchor Version Lookup**:
   * For each primary key in the table, the engine calls `HistoryIndex::latestBefore(pk, targetMs)`.
   * Binary search locates the newest version created on or before `targetMs`.
3. **Tombstone Filtering**:
   * If the anchor version has `deleted == 1`, the row is treated as non-existent at that point in time.
   * If `deleted == 0`, the record payload is read from disk at `version.offset`.
4. **WHERE & Projection**: Any `WHERE` conditions, `ORDER BY`, `LIMIT`, and column projections are applied to the snapshot records.

---

## Examples

### 1. Query Entire Table As Of a Past Date (`employees`)

```sql
SELECT * FROM employees AS OF 2026-01-01;
```

### 2. Query Specific Row As Of an Exact Timestamp (`employees`)

```sql
SELECT * FROM employees AS OF 2026-06-15 14:30:00 WHERE id = 1;
```

### 3. Using SNAPSHOT Keyword (`employees`)

```sql
SELECT id, title FROM employees SNAPSHOT 2026-08-01 WHERE id = 1;
```

---

## Related Commands

* [`BETWEEN`](between.md) — Query all versions in a date interval.
* [`HISTORY`](history.md) — View all versions of a specific record.
* [`ROLLBACK`](rollback.md) — Restore a past state.
