# DROP TABLE

## Description

The `DROP TABLE` statement unmounts a table from the catalog and permanently removes its directory and data files from disk (`data/<table_name>/`).

---

## Syntax

*(See the [Syntax Notation Guide](../reference/grammar.md#syntax-notation-guide) for notation rules).*

### Basic syntax

```sql
DROP TABLE <table>;
```

### Full syntax

```sql
DROP TABLE <table>;
```

---

## Parameters

| Parameter | Description |
|---|---|
| `<table>` | The name of the table to drop. |

---

## Example

```sql
DROP TABLE documents;
```

---

## Expected Output

```text
Dropped table 'documents'
```

---

## How It Works

1. **Catalog Lookup**: Verifies that the table is currently mounted in the catalog.
2. **Vector Cleanup**: Erases any in-memory vector index handles associated with the table.
3. **Filesystem Removal**: Calls `std::filesystem::remove_all("data/" + tableName)` to delete `data.db`, `<table_name>.vec`, and the `archive/` directory.
4. **Catalog Unregistration**: Removes the table entry from `Catalog::tables`.

---

## Errors & Edge Cases

| Error Condition | Error Message |
|---|---|
| Table Does Not Exist | `No such table: '<table>'` |
| Filesystem Deletion Failure | `Failed to drop table '<table>'` |

---

## Related Commands

* [`CREATE TABLE`](create-table.md) — Create a table.
* [`SHOW TABLES`](show-tables.md) — Check remaining tables after dropping.
