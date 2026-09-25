# HISTORY

## Description

The `HISTORY` statement displays the complete version timeline for one or more rows. It retrieves every record version ever committed to disk for the matching row, including the initial `INSERT`, subsequent `UPDATE`s, and any `DELETE` tombstone.

---

## Syntax

*(See the [Syntax Notation Guide](../reference/grammar.md#syntax-notation-guide) for notation rules).*

### Basic syntax

```sql
HISTORY <table> WHERE <condition>;
```

### Full syntax

```sql
HISTORY <table> WHERE <condition>;
```

> [!IMPORTANT]
> A `WHERE` clause is **required**. You cannot execute a bare `HISTORY <table>;` without a condition.

---

## Parameters

| Parameter | Description |
|---|---|
| `<table>` | Target table name. |
| `WHERE <condition>` | A filter condition identifying the target row (typically `<pk_col> = <val>`). |

---

## Example

```sql
HISTORY employees WHERE id = 1;
```

---

## Output Structure

The output displays each version in chronological order, showing:
1. `Timestamp`: High-resolution creation time (`YYYY-MM-DD HH:MM:SS.mmm`).
2. `Deleted`: Tombstone flag (`NO` for active records, `YES` for deleted records).
3. Column values as they existed in that specific version.

*(Note: Timestamps reflect the actual system time when each record was inserted or updated).*

```text
---------------------------------------------------------------------------------------------------------
Timestamp                Deleted  id   name           title              salary
---------------------------------------------------------------------------------------------------------
[Runtime Timestamp 1]    NO       1    Elena Rostova  Junior Engineer    75000.00
[Runtime Timestamp 2]    NO       1    Elena Rostova  Software Engineer  95000.00
[Runtime Timestamp 3]    YES      1    Elena Rostova  Software Engineer  95000.00
---------------------------------------------------------------------------------------------------------
3 version(s) across 1 row(s)
```

In this output:
* Version 1 represents the initial `INSERT`.
* Version 2 represents an `UPDATE` that changed the title and salary.
* Version 3 represents a subsequent `DELETE` (marked with `Deleted = YES`).

---

## How It Works

1. **Resolve Primary Keys**: The executor evaluates the `WHERE` condition to find all matching primary keys.
2. **Retrieve Version Chains**: For each primary key, `Table::showHistory(pk)` iterates through all `RecordVersion` entries in the `HistoryIndex`.
3. **Read Records from Disk**: For each version, the executor reads the record from its byte offset in `data.db` and prints the formatted timestamp, tombstone flag, and column payload.

---

## Errors & Edge Cases

| Error Condition | Error Message |
|---|---|
| Missing WHERE Clause | Parse error (`after table name in HISTORY (WHERE is required here)`). |
| Unknown Column in WHERE | `Unknown column '<col>' on table '<table>'` |
| No Matching Rows | `No rows matched` |

---

## Related Commands

* [`EVOLUTION`](evolution.md) — View step-by-step field diffs between consecutive versions.
* [`COMPARE`](compare.md) — View net field differences between two points in time.
* [`ROLLBACK`](rollback.md) — Restore a past version from history.
