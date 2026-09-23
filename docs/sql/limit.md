# LIMIT

## Description

The `LIMIT` clause restricts the maximum number of records returned by a `SELECT` statement.

---

## Syntax

*(See the [Syntax Notation Guide](../reference/grammar.md#syntax-notation-guide) for notation rules).*

### Basic syntax

```sql
SELECT ... FROM <table> LIMIT <n>;
```

### Full syntax

```sql
SELECT ... FROM <table> [...] LIMIT <n>;
```

---

## Parameters

| Parameter | Description |
|---|---|
| `<n>` | A non-negative integer specifying the maximum row count to return. |

---

## Behavior Across Query Types

### 1. Relational Queries
In standard relational queries, `LIMIT` truncates the resulting vector of records to the first `<n>` rows after any `WHERE` filtering and `ORDER BY` sorting have been applied.

```sql
SELECT * FROM students ORDER BY gpa DESC LIMIT 5;
```

### 2. Semantic Queries (`SIMILAR TO`)
In semantic search queries, `LIMIT` sets the capacity `k` of the top-k min-heap:
* If `LIMIT` is specified: Returns the top `<n>` most similar records.
* If `LIMIT` is **omitted**: The semantic engine defaults to returning the top **10** records.

```sql
-- Returns top 3 most semantically similar documents
SELECT * FROM documents WHERE content SIMILAR TO "network timeout" LIMIT 3;

-- Returns top 10 by default
SELECT * FROM documents WHERE content SIMILAR TO "network timeout";
```

---

## Errors & Edge Cases

| Error Condition | Error Message |
|---|---|
| Missing Integer | `Expected an integer literal after LIMIT` |

---

## Related Commands

* [`SELECT`](select.md) — Query records.
* [`ORDER BY`](order-by.md) — Order rows prior to applying LIMIT.
* [`SIMILAR TO`](../semantic/similar-to.md) — Semantic search ranking.
