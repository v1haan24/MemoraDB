# WHERE Clause

## Description

The `WHERE` clause filters rows in `SELECT`, `UPDATE`, `DELETE`, and temporal statements (`COMPARE`, `EVOLUTION`, `HISTORY`, `ROLLBACK`). It supports relational comparison operators as well as semantic similarity search.

---

## Syntax

*(See the [Syntax Notation Guide](../reference/grammar.md#syntax-notation-guide) for notation rules).*

### Basic syntax

```sql
WHERE <column> = <value>
```

### Full syntax

```sql
-- Relational comparison
WHERE <column> (= | != | < | <= | > | >=) <value>

-- Semantic search (SELECT only)
WHERE <semantic_col> SIMILAR TO "<query text>"
```

Supported comparison operators:

| Operator | Meaning | Applicable Data Types |
|---|---|---|
| `=` | Equal to | `INT`, `FLOAT`, `STRING`, `BOOL` |
| `!=` | Not equal to | `INT`, `FLOAT`, `STRING`, `BOOL` |
| `<` | Less than | `INT`, `FLOAT`, `STRING` |
| `<=` | Less than or equal to | `INT`, `FLOAT`, `STRING` |
| `>` | Greater than | `INT`, `FLOAT`, `STRING` |
| `>=` | Greater than or equal to | `INT`, `FLOAT`, `STRING` |

> [!IMPORTANT]
> `SIMILAR TO` is supported **only** in `SELECT` statements, and **only** on columns declared with the `SEMANTIC` modifier in `CREATE TABLE`.

---

## Language Rules & Constraints

### 1. Single Condition Only
MemoraDB supports **exactly one** condition per `WHERE` clause. Compound logical operations with `AND` or `OR` chains are **not** supported:

```sql
-- Valid
SELECT * FROM students WHERE gpa >= 3.5;

-- Invalid: compound conditions are not supported
SELECT * FROM students WHERE gpa >= 3.5 AND is_enrolled = true;
-- Parse Error
```

### 2. Boolean Type Comparisons
For `BOOL` columns, only `=` and `!=` are valid. Relational ordering operators (`<`, `<=`, `>`, `>=`) evaluate to `false` for boolean fields.

---

## Examples

### 1. Integer Comparison (`students`)

```sql
SELECT * FROM students WHERE id = 1;
```

### 2. Floating-Point Comparison (`students`)

```sql
SELECT * FROM students WHERE gpa >= 3.5;
```

### 3. String Comparison (`students`)

```sql
SELECT * FROM students WHERE name = "Alice Chen";
```

### 4. Semantic Search (`documents`)

```sql
SELECT * FROM documents WHERE content SIMILAR TO "SSL certificate expiration" LIMIT 5;
```

---

## Errors & Edge Cases

| Error Condition | Error Message |
|---|---|
| Unknown Column | `Unknown column '<col>' on table '<table>'` |
| SIMILAR TO on Non-Semantic Column | `Column '<col>' is not declared SEMANTIC, so SIMILAR TO can't be used on it` |
| SIMILAR TO in DML | `SIMILAR TO is only supported in SELECT ... WHERE on a SEMANTIC column` |
| Invalid Operator | `Expected a comparison operator (=, !=, <, <=, >, >=) but got ...` |

---

## Related Commands

* [`SELECT`](select.md) — Querying with WHERE.
* [`UPDATE`](update.md) — Targeted updates.
* [`DELETE`](delete.md) — Targeted deletions.
* [`SIMILAR TO`](../semantic/similar-to.md) — Semantic search reference.
