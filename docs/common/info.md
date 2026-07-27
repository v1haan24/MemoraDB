# Common Module

## Purpose

The Common module contains shared data structures, constants, and binary serialization helpers used across MemoraDB. Every major component (Catalog, Storage, History Index, and Temporal Engine) depends on this module.

---

## Files

### constants.h

Defines compile-time constants such as fixed-size limits and record/header sizes used throughout the database.

### metadata.h

Defines the core metadata structures representing tables, columns, rows, records, and provides helper functions for binary serialization/deserialization.

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

The metadata header is written once during table creation, while records are appended throughout the lifetime of the table.

---

## Metadata Header

```
+------------------------------+
| Metadata Size (4 B)          |
| Table Name (30 B)            |
| Payload Size (4 B)           |
| Column Count (4 B)           |
| Column Metadata × N          |
+------------------------------+
```

### Column Metadata

Each column stores:

- Column Name
- Data Type
- Storage Size
- Payload Offset
- Primary Key Flag

The metadata header completely describes the table schema and is used during record deserialization and crash recovery.

---

## Record Layout

Every record consists of a fixed-size header followed by the serialized row payload.

```
+----------------------+
| Timestamp (8 B)      |
| Deleted Flag (1 B)   |
+----------------------+
| Payload              |
+----------------------+
```

### Record Header

- **Timestamp** – Creation time of this version, used by temporal queries.
- **Deleted Flag** – Tombstone indicating whether the row is logically deleted.

### Payload

Stores serialized column values in the same order as defined in the table schema. The payload size is fixed for every row and is determined by the metadata header.

---

## Append-Only Storage

Records are never modified in-place.

- **INSERT** → Appends a new record.
- **UPDATE** → Appends a new version.
- **DELETE** → Appends a tombstone.
- **ROLLBACK** → Appends a restored historical version.

This immutable design preserves complete row history while enabling efficient crash recovery and temporal queries.