# COMPARE

## Description

The `COMPARE` statement computes the **net field difference** between the earliest version and the latest version of a row within a specified time range. Unlike `EVOLUTION` (which lists every intermediate change), `COMPARE` collapses intermediate edits and shows only the net changes between the start and end of the interval.

---

## Syntax

*(See the [Syntax Notation Guide](../reference/grammar.md#syntax-notation-guide) for notation rules).*

### Basic syntax

```sql
COMPARE <table> WHERE <condition> BETWEEN <date1> AND <date2>;
```

### Full syntax

```sql
COMPARE <table> WHERE <condition> BETWEEN <date1> AND <date2>;
```

> [!IMPORTANT]
> **Strict Clause Order**: In `COMPARE`, `WHERE` must strictly precede `BETWEEN ... AND ...`. Both clauses are mandatory.

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

Suppose employee `1` had multiple updates between January and December:
* Initial salary was `75000.0`
* Mid-year salary changed to `95000.0`
* End-of-year salary changed to `125000.0`

Running `COMPARE`:

```sql
COMPARE employees WHERE id = 1 BETWEEN 2026-01-01 AND 2026-12-31;
```

---

## Expected Output

```text
----------------------------------------------------------------------------------
Column               Before               After
----------------------------------------------------------------------------------
salary               75000.00             125000.00
----------------------------------------------------------------------------------
1 difference(s) across 1 row(s)
```

The intermediate update to `95000.00` is bypassed; only the net delta from `75000.00` to `125000.00` is shown.

---

## How It Works

1. **Retrieve Range Records**: Calls `Table::selectBetween(pk, t1, t2)`.
2. **Select Endpoints**: Selects the first version in the range (`recs.front()`) and the last version in the range (`recs.back()`).
3. **Field-by-Field Diff**: Calls `compareRecords(front, back, meta)`. If a field's value in `front` does not match `back`, a `Difference` record is added containing `column`, `before`, and `after`.

---

## Errors & Edge Cases

| Error Condition | Error Message |
|---|---|
| Range Start After End | `COMPARE range starts after it ends` |
| Missing WHERE or BETWEEN | Parse error (`after the WHERE condition in COMPARE`). |
| No Matching Rows | `No rows matched` |

---

## Related Commands

* [`EVOLUTION`](evolution.md) — View all intermediate step-by-step diffs.
* [`HISTORY`](history.md) — View full version timeline.
