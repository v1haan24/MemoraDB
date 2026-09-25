# Database Basics

This page provides an educational overview of core relational database concepts and details how MemoraDB specifically implements them.

---

## Relational Concepts vs. MemoraDB Implementation

| Concept | General Database Theory | MemoraDB Implementation |
|---|---|---|
| **Database** | A managed collection of schemas, tables, views, and indexes. | A `data/` directory on disk managed by the in-memory `Catalog`. |
| **Table** | A structured relation consisting of rows and columns conforming to a schema. | A dedicated directory `data/<name>/` containing `data.db` and optional `<name>.vec`. |
| **Row / Tuple** | An individual instance of data within a table. | A fixed-size binary payload preceded by a 9-byte header (`timestamp` + `deleted`). |
| **Column / Attribute** | A named, typed field defined within the table schema. | Fixed-offset slice within the row payload (`ColMeta`). |
| **Primary Key** | An attribute or combination of attributes that uniquely identifies a row. | Exactly one primary key column per table (`isPK=1`), indexed in-memory via `HistoryIndex`. |
| **Schema** | The formal definition of table names, column names, types, and constraints. | Serialized binary header at the beginning of each `data.db` file (`TableMeta`). |

---

## Tables & Binary Storage Format

In MemoraDB, tables are not stored as human-readable text or JSON; they are encoded in a compact **binary layout**.

### 1. Table Metadata Header
Every `data.db` file begins with a metadata block written during `CREATE TABLE`:
* `metadataSize` (`uint32_t`): Total byte length of the header.
* `tableName` (`char[30]`): Fixed 30-byte string for table name (`tns = 30`).
* `payloadSize` (`uint32_t`): Fixed byte size of a single row's payload.
* `columnCount` (`uint32_t`): Number of columns.
* Column definitions (`ColMeta` array), each storing:
  * `name` (`char[30]`): Fixed 30-byte column name.
  * `isPK` (`uint8_t`): `1` if primary key, `0` otherwise.
  * `isSemantic` (`uint8_t`): `1` if semantic vector indexing is enabled.
  * `type` (`int32_t`): Enum representing `INT`, `FLOAT`, `STRING`, or `BOOL`.
  * `size` (`uint32_t`): Size in bytes.
  * `offset` (`uint32_t`): Byte offset of the column within the row payload.

### 2. Row Record Structure
Following the metadata header, records are appended sequentially. Every record consists of:
1. **Row Header** (`rhsz = 9 bytes`):
   * `timestamp` (`uint64_t`, 8 bytes): Epoch millisecond when the record was written.
   * `deleted` (`uint8_t`, 1 byte): Tombstone indicator (`0 = active`, `1 = deleted`).
2. **Payload** (`payloadSize` bytes):
   * The binary serialization of each column value in order of definition.

```text
┌───────────────────────────┬─────────────────────────────────────────────────────────┐
│     Metadata Header       │                  Record Sequence                        │
├───────────────────────────┼─────────────────────────────────┬───────────────────────┤
│ [size][name][cols...]     │ [timestamp][del][col1][col2]... │ [ts][del][c1][c2]...  │
│ 0 ... metadataSize        │ offset A                        │ offset B              │
└───────────────────────────┴─────────────────────────────────┴───────────────────────┘
```

---

## Supported Data Types

MemoraDB supports four primitive data types:

### 1. `INT`
* **Internal Representation**: 32-bit signed integer (`int32_t`).
* **Binary Size**: 4 bytes.
* **Range**: -2,147,483,648 to 2,147,483,647.

### 2. `FLOAT`
* **Internal Representation**: Single-precision IEEE 754 floating-point (`float`).
* **Binary Size**: 4 bytes.

### 3. `STRING(n)`
* **Internal Representation**: Fixed-width character buffer of length `n`.
* **Binary Size**: `n` bytes.
* **Constraints**: Requires an explicit size between 1 and system limits. Shorter strings are padded with null bytes (`\0`).

### 4. `BOOL`
* **Internal Representation**: 8-bit unsigned integer (`uint8_t`).
* **Binary Size**: 1 byte (`0 = false`, `1 = true`).
* **Literal Values**: Parsed from string representations `"true"` and `"false"`.

---

## Primary Keys in MemoraDB

In general relational theory, a primary key enforces entity integrity. In MemoraDB, the primary key plays two vital roles:

1. **Identity Across Time**: Because records are versioned, multiple records on disk will share the same primary key value. The primary key connects these records into a **version chain**.
2. **Indexing Target**: The in-memory `HistoryIndex` maintains an `unordered_map<string, vector<RecordVersion>>` keyed by primary key string. This index maps the primary key to its chronological list of file offsets and timestamps.

> [!IMPORTANT]
> Because primary keys define version lineage, MemoraDB prohibits `UPDATE` statements targeting the primary key column. To modify a primary key, you must `DELETE` the old row and `INSERT` the new row.
