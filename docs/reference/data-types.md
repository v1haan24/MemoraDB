# Data Types Reference

MemoraDB is a strongly-typed database system. All table columns must be declared with one of four supported data types during `CREATE TABLE`.

---

## Supported Types

| Type | C++ Representation | Binary Size | Permitted Literals | Description |
|---|---|---|---|---|
| `INT` | `int32_t` | 4 bytes | `42`, `-10`, `0` | 32-bit signed integer. |
| `FLOAT` | `float` | 4 bytes | `3.14`, `-0.05`, `100.0` | IEEE 754 single-precision floating point. |
| `STRING(n)` | `char[n]` | `n` bytes | `'text'`, `"text"` | Fixed-width string buffer of size `n`. Requires explicit size. |
| `BOOL` | `uint8_t` | 1 byte | `true`, `false` | 8-bit boolean flag (`0 = false`, `1 = true`). |

---

## Type Details

### 1. `INT`
* Range: -2,147,483,648 to 2,147,483,647.
* Supports unary minus (`-`) in SQL queries.
* Comparison operators supported: `=`, `!=`, `<`, `<=`, `>`, `>=`.
* Sorting: Numerically ordered.

### 2. `FLOAT`
* Precision: ~7 decimal digits of precision.
* Supports unary minus (`-`) in SQL queries.
* Comparison operators supported: `=`, `!=`, `<`, `<=`, `>`, `>=`.
* Sorting: Numerically ordered.

### 3. `STRING(n)`
* **Mandatory Size**: Must be declared with an explicit capacity, e.g., `STRING(50)`.
* Storage: Fixed-size buffer on disk. Strings shorter than `n` bytes are null-padded; strings longer than `n` are rejected during validation.
* Quoting: Can be enclosed in single quotes (`'...'`) or double quotes (`"..."`).
* Comparison operators supported: `=`, `!=`, `<`, `<=`, `>`, `>=` (lexicographical).
* Special Modifier: Can be marked with `SEMANTIC` to enable AI vector search.

### 4. `BOOL`
* Accepted values: `true` or `false` (case-sensitive literal strings).
* Comparison operators supported: `=` and `!=` only. Ordering operators (`<`, `<=`, `>`, `>=`) evaluate to `false`.
* Sorting: `false` precedes `true` in ascending order.

---

## Column Modifiers

Columns can be annotated with modifiers during `CREATE TABLE`:

### `PRIMARY KEY`
* **Constraint**: Exactly one column per table must be designated `PRIMARY KEY`.
* **Immutability**: The primary key value cannot be modified via `UPDATE`.
* **Indexing**: Automatically indexed in-memory by `HistoryIndex`.
* **Valid Types**: Any supported type (`INT`, `STRING`, `FLOAT`, `BOOL`).

### `SEMANTIC`
* **Constraint**: Valid **only** on `STRING` columns.
* **Vector Model**: Generates a 384-dimensional vector embedding for the text using `all-MiniLM-L6-v2`.
* **Storage**: Vector embeddings are saved to a dedicated `.vec` file and indexed by `VectorIndex`.
* **Querying**: Enables the `SIMILAR TO` operator in `SELECT` statements.

---

## Identifier Naming Limits

MemoraDB enforces fixed buffer sizes for schema identifier names in metadata headers:

| Identifier Type | Constant | Constant Name | Buffer Size | Maximum Character Length |
|---|---|---|---|---|
| **Table Name** | `tns` | Table Name Size | 30 bytes | **29 characters** (+ 1 null terminator `\0`) |
| **Column Name** | `cns` | Column Name Size | 30 bytes | **29 characters** (+ 1 null terminator `\0`) |

> [!NOTE]
> Names exceeding 29 characters are rejected by the catalog validation logic during `CREATE TABLE`. Identifiers must start with an alphabetic character or underscore (`_`).

