# BETWEEN

## Description

The `BETWEEN` clause retrieves all versions of records committed within a specified time interval `[date1, date2]`. Unlike `AS OF` (which returns only one snapshot version per row), `BETWEEN` can return multiple historical versions for each row that was modified during the window.

---

## Syntax

*(See the [Syntax Notation Guide](../reference/grammar.md#syntax-notation-guide) for notation rules).*

### Basic syntax

```sql
SELECT * FROM <table> BETWEEN <date1> AND <date2>;
```

### Full syntax

```sql
SELECT * | <columns> 
FROM <table> 
BETWEEN <date1> AND <date2> 
[WHERE <condition>]
[ORDER BY <col> [ASC|DESC]]
[LIMIT <n>];
```

> [!NOTE]
> In `SELECT` statements, the temporal clause and `WHERE` clause can appear in either order:
> ```sql
> SELECT * FROM employees BETWEEN 2026-01-01 AND 2026-06-01 WHERE id = 1;
> SELECT * FROM employees WHERE id = 1 BETWEEN 2026-01-01 AND 2026-06-01;
> ```

---

## Boundary Semantics

* **Start Boundary (`<date1>`)**: Interpreted via `dayStartMs`. If no time is specified, it defaults to the **beginning of the day** (`00:00:00.000`).
* **End Boundary (`<date2>`)**: Interpreted via `dayEndMs`. If no time is specified, it defaults to the **end of the day** (`23:59:59.999`).

This ensures that writing `BETWEEN 2026-01-01 AND 2026-01-31` covers the entire calendar month of January from the first millisecond to the last.

---

## How It Works

1. **Range Conversion**: Converts both dates to millisecond timestamps `t1` and `t2`. If `t1 > t2`, an error is returned immediately.
2. **Version Retrieval**:
   * For each primary key, `Table::selectBetween(pk, t1, t2)` locates the first version committed at or before `t1` (`anchor`).
   * Iterates through all subsequent versions whose `timestamp <= t2`.
   * Skips records marked with the `deleted` tombstone.
3. **Filtering & Output**: Applies any `WHERE` condition and column projections.

---

## Examples

### 1. View All Row Changes Across a Quarter (`employees`)

```sql
SELECT * FROM employees BETWEEN 2026-01-01 AND 2026-03-31;
```

### 2. Track Single Row Revisions (`employees`)

```sql
SELECT * FROM employees 
BETWEEN 2026-06-01 AND 2026-06-30 
WHERE id = 1;
```

### 3. Combined with Semantic Search (`documents`)

```sql
SELECT * FROM documents 
BETWEEN 2026-01-01 AND 2026-06-01 
WHERE content SIMILAR TO "authentication timeout" 
LIMIT 5;
```

---

## Errors & Edge Cases

| Error Condition | Error Message |
|---|---|
| Start Date After End Date | `BETWEEN range starts after it ends` |

---

## Related Commands

* [`AS OF`](as-of.md) — Single point-in-time snapshot.
* [`HISTORY`](history.md) — View full version history for a specific row.
* [`COMPARE`](compare.md) — View net differences over an interval.
* [`EVOLUTION`](evolution.md) — View step-by-step diffs over an interval.
