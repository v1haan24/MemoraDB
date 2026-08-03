# metadata.h

## Purpose

Defines all shared data structures used throughout MemoraDB.

These structures describe table schemas, rows, record versions, and helper serialization functions.

Almost every module depends on this file.

---

## Structures

### ColMeta

Represents metadata for a single column.

Stores:

- Column Name
- Data Type
- Storage Size
- Payload Offset
- Primary Key Flag

The offset is calculated during table creation and allows direct access to any column inside a binary record.

---

### TableMeta

Represents the schema of an entire table.

Stores:

- Table Name
- Metadata Size
- Payload Size
- Number of Columns
- List of Column Metadata

The metadata header is written once when a table is created and read back during crash recovery.

---

### Row

Represents a logical row.

Internally stores all values as strings.

These strings are validated and converted into binary values during serialization.

---

### Record

Represents one version of a row.

Contains:

- Row Data
- Deleted Flag
- Timestamp

Used by all temporal queries.

---

### RecordVersion

Represents an entry inside the History Index.

Stores only

- Timestamp
- File Offset

instead of storing the complete row.

The actual row remains inside the append-only database file.

---

## Serialization Helpers

### writeBinary()

Writes any trivially-copyable object directly into a binary stream.

Used throughout metadata and record serialization.

---

### readBinary()

Reads a binary object from a stream.

Acts as the counterpart of writeBinary().

---

### writeColumn()

Serializes one column's metadata into the table header.

Writes:

- Name
- Type
- Size
- Offset
- Primary Key Flag

---

### readColumn()

Reads a serialized column definition from the database file and reconstructs a ColMeta object.

---

## Notes

This file defines the binary layout of the database.

Any structural modification here changes the on-disk file format and requires updating the recovery logic.