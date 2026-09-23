# EVOLUTION

## Description

The `EVOLUTION` statement tracks the step-by-step field changes of a row over a time interval. It compares each consecutive pair of versions within the specified range, producing a chronological changelog of what was modified, when it was modified, and what the previous and new values were.

---

## Syntax

*(See the [Syntax Notation Guide](../reference/grammar.md#syntax-notation-guide) for notation rules).*

### Basic syntax

```sql
EVOLUTION <table> WHERE <condition> BETWEEN <date1> AND <date2>;
```

### Full syntax

```sql
EVOLUTION <table> WHERE <condition> BETWEEN <date1> AND <date2>;
```

> [!IMPORTANT]
> **Strict Clause Order**: In `EVOLUTION`, `WHERE` must strictly precede `BETWEEN ... AND ...`. Both clauses are mandatory.

---

## Parameters

| Parameter | Description |
|---|---|
| `<table>` | Target table name. |
| `WHERE <condition>` | Filter condition identifying the target row (e.g., `id = 1`). |
| `<date1>` | Starting date or timestamp of the observation interval. |
| `<date2>` | Ending date or timestamp of the observation interval. |

---

## Example (`employees`)

```sql
EVOLUTION employees WHERE id = 1 BETWEEN 2026-01-01 AND 2026-12-31;
```

---

## Output Structure

The output displays each modified column across consecutive version pairs:
1. `Timestamp`: Time when the change was committed.
2. `Column`: Name of the modified attribute (or `Deleted` if tombstone status changed).
3. `Before`: Value prior to the change.
4. `After`: Value after the change.

*(Note: Timestamps reflect the actual system time when each modification occurred).*

```text
---------------------------------------------------------------------------------------------------------
Timestamp                Column               Before                    After
---------------------------------------------------------------------------------------------------------
[Runtime Timestamp 1]    title                Junior Engineer           Software Engineer
[Runtime Timestamp 1]    salary               75000.00                  95000.00
[Runtime Timestamp 2]    Deleted              false                     true
---------------------------------------------------------------------------------------------------------
3 change(s) across 1 row(s)
```

In this changelog:
1. At `[Runtime Timestamp 1]`, the `title` and `salary` columns changed from their initial values to their new values.
2. At `[Runtime Timestamp 2]`, the row was deleted (the `Deleted` flag changed from `false` to `true`).

---

## How It Works

1. **Resolve Primary Key**: The engine locates the row's primary key using the `WHERE` condition.
2. **Locate Start Version**: Finds the anchor version active at `t1` (`latestBefore(pk, t1)`).
3. **Sequential Pairwise Comparison**:
   * For each consecutive version pair `(r1, r2)` in the history log up to `t2`:
   * Calls `compareRecords(r1, r2, meta)`.
   * For each column where values differ (or if `deleted` changed), creates a `Difference` entry stamped with `r2.timestamp`.
4. **Return Changelog**: Returns the ordered list of differences to the user.

---

## Errors & Edge Cases

| Error Condition | Error Message |
|---|---|
| Range Start After End | `EVOLUTION range starts after it ends` |
| Missing WHERE or BETWEEN | Parse error (`after the WHERE condition in EVOLUTION`). |
| No Matching Rows | `No rows matched` |

---

## Related Commands

* [`COMPARE`](compare.md) — View the net difference between the start and end points directly.
* [`HISTORY`](history.md) — View raw versions without pairwise diff calculation.
