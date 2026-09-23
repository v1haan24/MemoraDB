# Temporal Database Overview

MemoraDB is architected from the ground up as an **append-only temporal database**. Instead of treating past data as an ephemeral byproduct destined to be overwritten, MemoraDB makes historical state an actively queryable, first-class dimension.

---

## How Temporal Data Works in MemoraDB

Every data modification (`INSERT`, `UPDATE`, `DELETE`) appends a new record version to the table's `data.db` file with a 64-bit millisecond timestamp (`uint64_t timestamp`).

```text
Version 1 (Initial Insert)  ──►  Timestamp: T1 (Offset: 0x0080)
      │
      ▼ (UPDATE)
Version 2 (Salary Modified) ──►  Timestamp: T2 (Offset: 0x0120)
      │
      ▼ (UPDATE)
Version 3 (Title Modified)  ──►  Timestamp: T3 (Offset: 0x01C0)
      │
      ▼ (DELETE)
Version 4 (Tombstone)       ──►  Timestamp: T4 (Offset: 0x0260)
```

### The In-Memory History Index

To provide fast lookups without scanning the entire `data.db` file on every query, MemoraDB maintains an in-memory `HistoryIndex`:

* **Data Structure**: `std::unordered_map<std::string, std::vector<RecordVersion>>`
* **Ordering**: For each primary key, versions are stored in strictly increasing chronological order.
* **Point-in-Time Resolution**: `HistoryIndex::latestBefore(pk, timestamp)` uses binary search (`O(log V)`) to locate the active record version as of any historical instant.

---

## Summary of Temporal Statements

| Statement | Purpose | Description |
|---|---|---|
| [`AS OF / SNAPSHOT`](as-of.md) | Point-in-Time Query | Reconstructs the exact state of rows as they existed at a specific past date or instant. |
| [`BETWEEN ... AND ...`](between.md) | Interval Query | Retrieves all record versions committed within a time window. |
| [`HISTORY`](history.md) | Full Timeline | Displays the complete lifecycle of versions for a specific row. |
| [`EVOLUTION`](evolution.md) | Step-by-Step Changes | Shows the sequential, pairwise field diffs between consecutive versions over time. |
| [`COMPARE`](compare.md) | Net Difference | Computes the net field differences between the earliest and latest versions in a range. |
| [`ROLLBACK`](rollback.md) | Non-Destructive Restore | Restores past state by appending historical records as new versions. |
| [`COMPACT TABLE`](compact.md) | History Pruning & Archival | Purges superseded versions prior to an anchor date and archives old data safely. |
| [`Date & Timestamp Literals`](timestamps.md) | Syntax Specification | Syntax rules, precision levels (ms), and defaulting semantics for timestamps. |

---

## Flexible Clause Ordering

MemoraDB allows combining temporal clauses with relational `WHERE` clauses in either order:

```sql
SELECT * FROM notes AS OF 2026-09-01 WHERE id = 1;
SELECT * FROM notes WHERE id = 1 AS OF 2026-09-01;
```

Both queries parse and produce identical results.
