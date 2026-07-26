# metadata.h

## Purpose

Defines the core data structures used throughout MemoraDB.

These structures describe both the database schema and the runtime objects manipulated by the storage engine.

---

## Enumerations

### DataType

Represents the supported column data types.

Supported values:

- INT
- FLOAT
- STRING
- BOOL

Used during:

- Validation
- Serialization
- Deserialization

---

## Structures

### ColMeta

Represents the metadata of a single column.

Members:

- name
- type
- size
- offset
- isPK

Responsibilities:

- Defines column layout
- Stores fixed storage size
- Stores byte offset inside a row payload

---

### TableMeta

Represents the metadata of an entire table.

Members:

- metadataSize
- name
- payloadSize
- columnCount
- columns

Responsibilities:

- Describes the complete table schema
- Used during table creation
- Used during recovery
- Used during serialization/deserialization

---

### Row

Represents one logical database row.

Members:

- values

Responsibilities:

- Stores user-visible data
- Used by CRUD operations
- Converted into binary payload during serialization

---

### Record

Represents one physical version stored on disk.

Members:

- timestamp
- deleted
- row

Responsibilities:

- Stores immutable row versions
- Enables temporal queries
- Supports rollback and history

---

### VersionPointer

Represents one entry inside the History Index.

Members:

- timestamp
- offset

Responsibilities:

- Maps a record version to its location on disk
- Enables O(1) latest lookups
- Enables temporal reconstruction

---

### Difference

Represents a change between two record versions.

Members:

- timestamp
- column
- before
- after

Responsibilities:

- Used by Compare()
- Used by Evolution()
- Displays field-level modifications between versions

---

## Relationships

```
TableMeta
│
├── ColMeta
│
└── Row
     │
     ▼
Record
     │
     ▼
VersionPointer
```

---

## Used By

- Catalog
- Table
- History Index
- Recovery
- Temporal Engine
- Serialization