# Common Module

## Purpose

The Common module contains shared data structures, constants, and binary serialization helpers used throughout MemoraDB. Every major component (Catalog, Storage, History Index, and Temporal Engine) depends on this module.

---

## Files

### constants.h

Defines compile-time constants such as fixed-size limits, name lengths, and record/header sizes used across the database.

### metadata.h

Defines the core metadata structures representing tables, columns, rows, records, and provides helper functions for binary serialization and deserialization.

---

# Database File Structure

Each table is stored in its own binary `.db` file.

```
+----------------------+
| Metadata Header      |
+----------------------+
| Record 1             |
+----------------------+
| Record 2             |
+----------------------+
| ...                  |
+----------------------+
```

The database file begins with a metadata header followed by an append-only sequence of records. The metadata is written once during table creation, while records are continuously appended throughout the table's lifetime.

---

## Metadata Header

The metadata header completely describes the table schema and is used during record deserialization and crash recovery.

```
+--------------------------------------+
| Metadata Size      (4 B)             |
| Table Name         (tns = 30 B)      |
| Payload Size       (4 B)             |
| Column Count       (4 B)             |
| Column Metadata × N                  |
+--------------------------------------+
```

Where:

- **Metadata Size (4 B)** – Total size of the metadata header. Used to directly locate the first record during recovery.
- **Table Name (30 B)** – Fixed-length character array storing the table name.
- **Payload Size (4 B)** – Size (in bytes) of the serialized row payload. The 9-byte record header (Timestamp + Deleted Flag) is not included.
- **Column Count (4 B)** – Total number of columns present in the table.
- **Column Metadata × N** – One metadata block is stored for every column in the table.

### Column Metadata

Each column contributes the following metadata block:

```
+--------------------------------------+
| Column Name        (cns = 30 B)      |
| Data Type          (4 B)             |
| Storage Size       (4 B)             |
| Payload Offset     (4 B)             |
| Primary Key Flag   (1 B)             |
+--------------------------------------+
```

Where:

- **Column Name (30 B)** – Fixed-length character array storing the column name.
- **Data Type (4 B)** – Column type (`INT`, `FLOAT`, `BOOL`, or `STRING`).
- **Storage Size (4 B)** – Number of bytes allocated for the field inside the payload.
- **Payload Offset (4 B)** – Starting byte offset of the field within the payload, allowing direct access without scanning previous columns.
- **Primary Key Flag (1 B)** – Indicates whether the column is the table's primary key.

---

## Record Layout

Each record represents one immutable version of a row. Every record consists of a fixed-size header followed by the serialized row payload.

```
+----------------------+
| Record Header (9 B)  |
+----------------------+
| Payload              |
+----------------------+
```

### Record Header

```
+----------------------+
| Timestamp (8 B)      |
| Deleted Flag (1 B)   |
+----------------------+
```

Where:

- **Timestamp** – Creation time of the record version. Used by temporal queries and crash recovery.
- **Deleted Flag** – Tombstone indicating whether the record is logically deleted.

### Payload

The payload stores serialized column values in the same order as defined by the table schema. Its size is fixed for a given table and is determined by the metadata header.

---

## Append-Only Storage

Records are immutable and are never modified in-place.

- **INSERT** → Appends a new record.
- **UPDATE** → Appends a new version of the row.
- **DELETE** → Appends a tombstone record.
- **ROLLBACK** → Appends a restored historical version.

This append-only design preserves complete row history while enabling efficient crash recovery and temporal queries.