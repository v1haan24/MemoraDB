# SHOW TABLES

## Description

The `SHOW TABLES` statement lists the names of all tables currently mounted in the MemoraDB catalog.

---

## Syntax

*(See the [Syntax Notation Guide](../reference/grammar.md#syntax-notation-guide) for notation rules).*

### Basic syntax

```sql
SHOW TABLES;
```

### Full syntax

```sql
SHOW TABLES;
```

---

## Parameters

None.

---

## Example

```sql
SHOW TABLES;
```

---

## Expected Output

When tables exist:
```text
documents
employees
students
```

When no tables are registered:
```text
No tables found.
```

---

## How It Works

The executor queries the `Catalog` map and prints the name of each mounted table. When MemoraDB starts up, `Catalog::loadTables()` scans the `data/` directory and mounts every valid table found on disk, so `SHOW TABLES` reflects both newly created tables and tables persisted from previous sessions.

---

## Related Commands

* [`CREATE TABLE`](create-table.md) — Create a new table.
* [`DESCRIBE TABLE`](describe-table.md) — Inspect the schema of a listed table.
* [`DROP TABLE`](drop-table.md) — Delete a table.
