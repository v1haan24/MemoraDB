# SELECT

## Description

The `SELECT` statement queries records from a table. It supports column projection, row filtering (`WHERE`), point-in-time and time-interval temporal queries (`AS OF`, `SNAPSHOT`, `BETWEEN`), semantic similarity search (`SIMILAR TO`), result sorting (`ORDER BY`), and pagination (`LIMIT`).

---

## Syntax

*(See the [Syntax Notation Guide](../reference/grammar.md#syntax-notation-guide) for notation rules).*

### Basic syntax

```sql
SELECT * FROM <table>;
```

### Full syntax

```sql
SELECT * | <col1>[, <col2> ...]
FROM <table>
[ <temporal-clause> ]
[ WHERE <condition> ]
[ ORDER BY <col> [ASC|DESC] ]
[ LIMIT <n> ];
```

Where `<temporal-clause>` can be:
```sql
AS OF <date>
SNAPSHOT <date>
BETWEEN <date1> AND <date2>
```

> [!NOTE]
> **Flexible Clause Ordering in SELECT**: The `<temporal-clause>` and `WHERE` clause are independent and can appear in **either order**:
> ```sql
> SELECT * FROM employees AS OF 2026-09-01 WHERE id = 1;
> SELECT * FROM employees WHERE id = 1 AS OF 2026-09-01;
> ```
> Both forms parse and execute identically in MemoraDB.

---

## Parameters

| Component | Description |
|---|---|
| `*` | Selects all columns in the table schema. |
| `<col1>, <col2>` | Explicit column projection. Only specified columns will be returned. |
| `FROM <table>` | The target table name. |
| `AS OF <date>` / `SNAPSHOT <date>` | Evaluates the table state as of the specified date or timestamp. |
| `BETWEEN <d1> AND <d2>` | Returns all record versions committed between `d1` (inclusive start) and `d2` (inclusive end). |
| `WHERE <condition>` | Filters records by a comparison (`=`, `!=`, `<`, `<=`, `>`, `>=`) or semantic similarity (`SIMILAR TO`). Only one condition is permitted per statement. |
| `ORDER BY <col> [ASC\|DESC]` | Sorts rows by `<col>`. Defaults to `ASC` if omitted. |
| `LIMIT <n>` | Limits output to the first `<n>` rows. |

---

## Execution Branches: Ordinary vs. Semantic

The executor routes `SELECT` into one of two execution paths based on the `WHERE` operator:

### 1. Semantic Search Branch (`SIMILAR TO`)
When `WHERE <col> SIMILAR TO "<text>"` is used:
* The column must have been declared `SEMANTIC` during `CREATE TABLE`.
* The query string is encoded into a 384-dimensional vector using the in-process ONNX model.
* Candidate records are filtered by temporal constraints (if specified).
* Cosine similarity is computed against each candidate's vector.
* A min-heap extracts the top-k highest scoring records (defaults to `k = 10` if `LIMIT` is omitted).
* Results display the primary key, version timestamp, similarity score (runtime-computed float), and semantic column values.

### 2. Ordinary Relational Branch
When `SIMILAR TO` is not present:
* Candidates are generated based on the temporal mode (`LATEST`, `SNAPSHOT`, or `BETWEEN`).
* If `WHERE` is present, rows are filtered using the comparison operator.
* If `ORDER BY` is present, records are sorted according to column data type.
* If `LIMIT` is present, the result vector is truncated.
* If specific columns were requested, rows are projected to include only those columns.

---

## Examples

### 1. Standard Relational Query (`students`)

```sql
SELECT id, name, gpa FROM students WHERE gpa >= 3.5 ORDER BY gpa DESC LIMIT 5;
```

### 2. Point-in-Time Temporal Query (`employees`)

```sql
SELECT * FROM employees AS OF 2026-01-15 WHERE id = 101;
```

### 3. Range-Based Temporal Query (`employees`)

```sql
SELECT * FROM employees BETWEEN 2026-01-01 AND 2026-06-30;
```

### 4. Semantic Search with Limit (`documents`)

```sql
SELECT * FROM documents WHERE content SIMILAR TO "database crash recovery" LIMIT 3;
```

### 5. Combined Temporal + Semantic Query (`documents`)

```sql
SELECT * FROM documents 
BETWEEN 2026-01-01 AND 2026-06-01 
WHERE content SIMILAR TO "container orchestration" 
LIMIT 5;
```

---

## Errors & Edge Cases

| Error Condition | Error Message |
|---|---|
| Unknown Column in Select List | `Unknown column '<col>' in the select list` |
| Unknown Column in WHERE | `Unknown column '<col>' on table '<table>'` |
| Unknown Column in ORDER BY | `Unknown column '<col>' in ORDER BY` |
| SIMILAR TO on Non-Semantic Column | `Column '<col>' is not declared SEMANTIC, so SIMILAR TO can't be used on it` |
| BETWEEN Start After End | `BETWEEN range starts after it ends` |

---

## Related Commands

* [`WHERE Clause`](where.md) — Detailed comparison and semantic operator rules.
* [`ORDER BY`](order-by.md) — Sorting semantics.
* [`LIMIT`](limit.md) — Result pagination.
* [`AS OF`](../temporal/as-of.md) — Temporal snapshot reference.
* [`SIMILAR TO`](../semantic/similar-to.md) — Semantic query reference.
