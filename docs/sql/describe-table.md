# DESCRIBE TABLE

## Description

The `DESCRIBE TABLE` statement prints the schema details of a specified table, including column names, data types, maximum byte sizes, primary key status, and semantic indexing status.

---

## Syntax

*(See the [Syntax Notation Guide](../reference/grammar.md#syntax-notation-guide) for notation rules).*

### Basic syntax

```sql
DESCRIBE TABLE <table>;
```

### Full syntax

```sql
DESCRIBE TABLE <table>;
```

---

## Parameters

| Parameter | Description |
|---|---|
| `<table>` | The name of the table to describe. |

---

## Example

```sql
DESCRIBE TABLE students;
```

---

## Expected Output

```text
Table: students
Columns: 4
--------------------------------------------------
Name                 Type         Size  PK  Semantic
--------------------------------------------------
id                   INT          4     YES NO
name                 STRING       50    NO  NO
gpa                  FLOAT        4     NO  NO
is_enrolled          BOOL         1     NO  NO
--------------------------------------------------
Described table 'students'
```

---

## Errors & Edge Cases

| Error Condition | Error Message |
|---|---|
| Table Does Not Exist | `No such table: '<table>'` |

---

## Related Commands

* [`CREATE TABLE`](create-table.md) — Define a table.
* [`SHOW TABLES`](show-tables.md) — List available tables.
