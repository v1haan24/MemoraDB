# CREATE TABLE

## Description

The `CREATE TABLE` statement registers a new table schema within the MemoraDB catalog and initializes its on-disk storage files (`data/<table_name>/data.db` and optional vector tables).

---

## Syntax

*(See the [Syntax Notation Guide](../reference/grammar.md#syntax-notation-guide) for notation rules).*

### Basic syntax

```sql
CREATE TABLE <table> (
    <col1> <TYPE> PRIMARY KEY,
    <col2> <TYPE>
);
```

### Full syntax

```sql
CREATE TABLE <table> (
    <col1> <TYPE>[(<size>)] [PRIMARY KEY] [SEMANTIC],
    <col2> <TYPE>[(<size>)] [PRIMARY KEY] [SEMANTIC],
    ...
);
```

---

## Parameters & Modifiers

| Component | Description | Rules & Constraints |
|---|---|---|
| `<table>` | Name of the table. | Up to 29 characters. Must be unique within the catalog. |
| `<col>` | Name of the column. | Up to 29 characters. Column names must be unique within the table. |
| `<TYPE>` | Data type. | One of `INT`, `FLOAT`, `STRING`, or `BOOL`. |
| `(<size>)` | Size specifier. | **Mandatory** for `STRING` columns (e.g., `STRING(50)`). Must be a positive integer. Not permitted on `INT`, `FLOAT`, or `BOOL`. |
| `PRIMARY KEY` | Primary key constraint. | **Exactly one** column per table must be designated `PRIMARY KEY`. |
| `SEMANTIC` | Vector embedding modifier. | Valid **only** on `STRING` columns. Instructs the engine to generate 384-dimensional embeddings for text inserted into this column. |

> [!NOTE]
> Modifiers are order-independent: `<col> STRING(100) PRIMARY KEY SEMANTIC` and `<col> STRING(100) SEMANTIC PRIMARY KEY` are both valid.

---

## Examples

### 1. Basic Relational Table (`students`)

```sql
CREATE TABLE students (
    id INT PRIMARY KEY,
    name STRING(50),
    gpa FLOAT,
    is_enrolled BOOL
);
```

### 2. Table with Semantic Search (`documents`)

```sql
CREATE TABLE documents (
    id INT PRIMARY KEY,
    title STRING(60),
    content STRING(500) SEMANTIC
);
```

---

## Expected Output

```text
Created table 'students' (4 columns)
Created table 'documents' (3 columns) with semantic index
```

---

## How It Works

1. **Validation**: The parser verifies column definitions, ensuring unique column names, a single primary key, and valid type sizing.
2. **Payload Calculation**: `Catalog::CalcOffset` computes byte offsets for each column within the fixed-size row payload.
3. **Directory & File Creation**:
   * Creates directory `data/<table>/`.
   * Creates `data/<table>/archive/` for historical archives.
   * Writes the binary `TableMeta` header into `data/<table>/data.db`.
4. **Semantic Vector Setup**: If any column has `SEMANTIC`, `vecMeta::createVecTable` creates `data/<table>/<table>.vec` with header metadata.
5. **Catalog Registration**: An in-memory `Table` instance is constructed and mounted into the catalog.

---

## Errors & Edge Cases

| Error Condition | Error Message |
|---|---|
| Missing Primary Key | `Exactly one primary key is required.` |
| Multiple Primary Keys | `Exactly one primary key is required.` |
| Duplicate Column Name | `Duplicate column name: <col>` |
| String Missing Size | `STRING column '<col>' needs a size, e.g. STRING(50)` |
| Semantic on Non-String | `SEMANTIC is only valid on STRING columns (column '<col>')` |
| Table Name Too Long | `Table name exceeds 29 characters.` |
| Column Name Too Long | `Column name '<col>' exceeds 29 characters.` |
| Table Already Exists | `Table '<table>' already exists.` |

---

## Related Commands

* [`DESCRIBE TABLE`](describe-table.md) — Inspect the columns and metadata of a created table.
* [`DROP TABLE`](drop-table.md) — Remove an existing table and its files.
* [`SHOW TABLES`](show-tables.md) — List all created tables.
