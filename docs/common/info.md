# Common Module

## Overview

The Common module contains shared data structures, constants, and serialization utilities used throughout MemoraDB.

Every major component of the database depends on this module, making it the foundation of the storage engine.

---

## Files

### metadata.h

Defines the core data structures used by MemoraDB, including table metadata, column metadata, rows, records, temporal differences, and version pointers.

---

### constants.h

Defines project-wide compile-time constants used by the storage engine, including fixed string sizes and binary layout constants.

---

### serialization.cpp

Provides helper functions for serializing and deserializing metadata, rows, and records between memory and disk.

---

## Used By

- Catalog
- Storage Engine
- History Index
- Recovery
- Temporal Engine
- Parser (future)
- Semantic Search (future)