# SQL Reference Overview

MemoraDB provides a custom SQL-like query language designed specifically for its append-only temporal and semantic architecture.

---

## Dialect Characteristics

Before writing queries in MemoraDB, note the following language rules:

* **Keyword Case-Insensitivity**: All SQL keywords are case-insensitive. `SELECT`, `select`, and `Select` are treated identically.
* **Identifier Case Sensitivity**: Identifiers (table names and column names) are matched against schema definitions. The parser will attempt exact match followed by lowercase fallback.
* **Semicolon Termination**: Every SQL statement **must** terminate with a semicolon (`;`). Statements without a semicolon will prompt the REPL for continuation lines (`...>`).
* **String Literals**: Strings can be quoted using either single quotes (`'text'`) or double quotes (`"text"`).
* **Numeric Literals**: Integers (`42`, `-10`) and floating-point numbers (`3.14`, `-0.05`) are natively supported, including unary minus.
* **No Inline SQL Comments**: The MemoraDB lexer does not include comment tokens (such as `--` or `/* */`). Do not include SQL comment markers in statements sent to the REPL.

---

## Supported Statement Categories

MemoraDB organizes statements into four functional categories:

### 1. Data Definition Language (DDL)

| Statement | Description |
|---|---|
| [`CREATE TABLE`](create-table.md) | Defines a new table schema with typed columns and primary key. |
| [`DROP TABLE`](drop-table.md) | Deletes a table and removes its data directory from disk. |
| [`DESCRIBE TABLE`](describe-table.md) | Prints the schema definition and column metadata of a table. |
| [`SHOW TABLES`](show-tables.md) | Lists all tables currently registered in the database catalog. |

### 2. Data Manipulation Language (DML)

| Statement | Description |
|---|---|
| [`INSERT INTO`](insert.md) | Appends a new row version with positional column values. |
| [`UPDATE`](update.md) | Appends a new row version with updated column values. |
| [`DELETE FROM`](delete.md) | Appends a tombstone record marking rows as deleted. |

### 3. Query Language (QL)

| Feature | Description |
|---|---|
| [`SELECT`](select.md) | Reads records with projection (`*` or column list), ordering, and limits. |
| [`WHERE Clause`](where.md) | Filters records using comparison operators or semantic search. |
| [`ORDER BY`](order-by.md) | Sorts query results by a specified column in ascending or descending order. |
| [`LIMIT`](limit.md) | Restricts the maximum number of records returned. |

### 4. Temporal Statements

| Statement | Description |
|---|---|
| [`AS OF / SNAPSHOT`](../temporal/as-of.md) | Queries the state of a table as of a past date or instant. |
| [`BETWEEN ... AND ...`](../temporal/between.md) | Queries all versions committed within a time window. |
| [`HISTORY`](../temporal/history.md) | Displays the full version chain for a specific primary key. |
| [`EVOLUTION`](../temporal/evolution.md) | Displays step-by-step column diffs across consecutive versions. |
| [`COMPARE`](../temporal/compare.md) | Displays net column differences between two points in time. |
| [`ROLLBACK`](../temporal/rollback.md) | Restores a row or table to a past state by appending new records. |
| [`COMPACT TABLE`](../temporal/compact.md) | Purges superseded history prior to an anchor date and archives old data. |
