# constants.h

## Purpose

Defines compile-time constants shared across the entire MemoraDB codebase.

Keeping these values centralized ensures a consistent binary file format and avoids duplicated magic numbers.

---

## Constants

### tns

Maximum number of bytes reserved for a table name.

Used during metadata serialization.

---

### cns

Maximum number of bytes reserved for a column name.

Also used as the fixed storage size for metadata column names.

---

### rhsz

Record Header Size.

Represents the size of every record header stored on disk.

Contents:

- Timestamp
- Deleted Flag

Used for:

- Recovery
- Updates
- Deletes
- Temporal Queries

---

## Why constants?

Using compile-time constants provides:

- Consistent binary layout
- Easier maintenance
- Single point of modification
- Reduced risk of serialization bugs

---

## Used By

- metadata.h
- serialization.cpp
- catalog.cpp
- recovery.cpp
- insert.cpp
- update.cpp
- delete.cpp