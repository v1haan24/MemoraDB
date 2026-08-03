# constants.h

## Purpose

Defines compile-time constants used throughout MemoraDB.

Keeping these values in one file ensures consistency across serialization, storage, and metadata handling.

---

## Constants

### cns 

Maximum number of characters allowed in a column name.

Used while serializing and deserializing table metadata.

---

### tns

Maximum number of characters allowed in a table name.

Stored directly inside the metadata header.

---

### rhsz

Size of every record header.

Represents

```
Timestamp + Deleted Flag
```

Used whenever calculating payload offsets inside the database file.

---

## Notes

Changing any of these values changes the binary layout of the database.

Existing database files may become incompatible if these constants are modified.