# ORDER BY

## Description

The `ORDER BY` clause sorts query results by a specified column in ascending (`ASC`) or descending (`DESC`) order.

---

## Syntax

*(See the [Syntax Notation Guide](../reference/grammar.md#syntax-notation-guide) for notation rules).*

### Basic syntax

```sql
SELECT ... FROM <table> ORDER BY <column>;
```

### Full syntax

```sql
SELECT ... FROM <table> [WHERE ...] ORDER BY <column> [ASC|DESC];
```

---

## Parameters

| Parameter | Description |
|---|---|
| `<column>` | The name of the column by which to sort results. |
| `ASC` | Sorts in ascending order (smallest to largest / alphabetical). This is the **default**. |
| `DESC` | Sorts in descending order (largest to smallest / reverse alphabetical). |

---

## Type-Aware Sorting Behavior

Sorting respects the underlying data type of the column:

* **`INT` Columns**: Numerically sorted by parsed integer value (`std::stoi`).
* **`FLOAT` Columns**: Numerically sorted by parsed float value (`std::stof`).
* **`STRING` Columns**: Lexicographically sorted by character code points.
* **`BOOL` Columns**: `false` sorts before `true` in ascending order.

---

## Examples

### 1. Default Ascending Sort (`students`)

```sql
SELECT * FROM students ORDER BY id;
```

### 2. Explicit Descending Sort (`students`)

```sql
SELECT * FROM students ORDER BY gpa DESC;
```

### 3. Combined with WHERE and LIMIT (`students`)

```sql
SELECT * FROM students WHERE gpa >= 3.0 ORDER BY gpa DESC LIMIT 3;
```

---

## Errors & Edge Cases

| Error Condition | Error Message |
|---|---|
| Unknown Column in ORDER BY | `Unknown column '<col>' in ORDER BY` |

---

## Related Commands

* [`SELECT`](select.md) — Main query statement.
* [`LIMIT`](limit.md) — Restrict result count.
