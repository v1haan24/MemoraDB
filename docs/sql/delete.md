# DELETE

## Description

The `DELETE FROM` statement soft-deletes matching rows from a table. In MemoraDB, a deletion does **not** erase the row from disk; instead, it appends a **tombstone record** (`deleted = 1`) to `data.db`. The row is excluded from standard queries but remains preserved for historical and temporal inspection.

---

## Syntax

*(See the [Syntax Notation Guide](../reference/grammar.md#syntax-notation-guide) for notation rules).*

### Basic syntax

```sql
DELETE FROM <table> WHERE <condition>;
```

### Full syntax

```sql
DELETE FROM <table> [WHERE <condition>];
```

---

## Parameters

| Parameter | Description |
|---|---|
| `<table>` | The target table name. |
| `WHERE <condition>` | Optional filter condition. If omitted, **every** active row in the table is deleted. |

---

## Examples

### 1. Delete Specific Row by Primary Key (`students`)

```sql
DELETE FROM students WHERE id = 1;
```

### 2. Delete with Comparison Condition (`students`)

```sql
DELETE FROM students WHERE gpa < 2.0;
```

### 3. Delete All Rows (`students`)

```sql
DELETE FROM students;
```

---

## Expected Output

```text
1 row(s) deleted
```

---

## How It Works

1. **Target Selection**: The executor scans the current active records (`table->scanLatest()`).
2. **WHERE Filtering**: If a `WHERE` clause is provided, matching records are identified.
3. **Tombstone Append**:
   * For each target record, `table->deleteRow(pk)` is invoked.
   * A new record is appended to the end of `data.db` with the `deleted` flag set to `1`.
   * The new version `{timestamp, offset}` is added to `HistoryIndex`.
4. **Subsequent Query Behavior**:
   * Standard `SELECT * FROM table;` ignores tombstoned records.
   * `SELECT * FROM table AS OF <past_date>;` will still return the record if it was active at `<past_date>`.
   * `HISTORY table WHERE pk = <val>;` displays the complete lifecycle, including the tombstone event.

---

## Errors & Edge Cases

| Error Condition | Error Message |
|---|---|
| Table Does Not Exist | `No such table: '<table>'` |
| Unknown Column in WHERE | `Unknown column '<col>' on table '<table>'` |
| Primary Key Not Found | If no active rows match the condition, `0 row(s) deleted` is returned. |

---

## Related Commands

* [`INSERT`](insert.md) — Insert records.
* [`UPDATE`](update.md) — Update records.
* [`HISTORY`](../temporal/history.md) — View deletion tombstones in row history.
* [`ROLLBACK`](../temporal/rollback.md) — Restore deleted rows back to life.
