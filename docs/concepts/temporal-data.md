# Temporal Data Concepts

This page explains temporal database principles, distinguishes different dimensions of time, and explains how MemoraDB implements transaction-time temporal data management.

---

## What is a Temporal Database?

Traditional database systems are **snapshot-oriented**: they store only the current state of reality. When an employee receives a promotion, an `UPDATE` statement overwrites their previous salary and title. The past state is destroyed unless custom audit tables or complex triggers were manually constructed.

A **Temporal Database** incorporates time as a first-class dimension. It preserves data as it evolves, allowing queries to inspect past states, analyze trends, compare versions, and audit changes without external logging infrastructure.

---

## Dimensions of Time: Valid Time vs. Transaction Time

Database literature defines two fundamental dimensions of time:

| Dimension | Definition | Controlled By | MemoraDB Support |
|---|---|---|---|
| **Valid Time** (Application Time) | The time period during which a fact is true in the real world (e.g., "policy effective from June 1 to Dec 31"). | User / Application | Handled via ordinary user-defined date columns if needed. |
| **Transaction Time** (System Time) | The exact physical instant when a fact was committed into the database storage engine. | Database Engine | **Core Feature**: Automatically captured as a 64-bit millisecond timestamp on every insert, update, and delete. |
| **Bitemporal** | Supports both Valid Time and Transaction Time simultaneously. | User + System | Not directly built-in; MemoraDB is a transaction-time temporal DBMS. |

MemoraDB is a **transaction-time temporal database**. When any mutation occurs, the DBMS stamps the record with `uint64_t timestamp` representing the system epoch time in milliseconds.

---

## The Append-Only Paradigm

Rather than modifying bytes in place or maintaining separate undo logs, MemoraDB uses an **append-only** architecture:

```text
Time Line: ──────────────────────────────────────────────────────────►

Record 1: [ID: 42, Status: "Pending", Timestamp: 1726800000000 (T1)]
     │
     ▼ (UPDATE status = "Active")
Record 2: [ID: 42, Status: "Active",  Timestamp: 1726886400000 (T2)]
     │
     ▼ (DELETE)
Record 3: [ID: 42, Status: "Active",  Timestamp: 1726972800000 (T3), DELETED=1]
```

### 1. Immutability
Once written to disk, a record's header and payload are never altered. Historical records remain indefinitely until an explicit `COMPACT TABLE` operation is issued.

### 2. Deletion via Tombstones
When a row is deleted:
* The data is **not** wiped from disk.
* A new record is appended with identical payload data, but the `deleted` flag in the record header is set to `1` (a **tombstone**).
* Standard queries treat tombstones as non-existent rows.
* Historical queries can still inspect when and what was deleted.

### 3. Chronological Version Chains
In memory, the `HistoryIndex` associates each primary key with an ordered list of `RecordVersion` structs:

```cpp
struct RecordVersion {
    uint64_t timestamp;  // Epoch milliseconds
    uint64_t offset;     // File offset in data.db
};
```

Because versions are appended in strictly increasing chronological order, finding the active record for any historical timestamp requires only a binary search (`O(log V)` where `V` is the number of versions for that key).

---

## Benefits of the MemoraDB Approach

1. **Deterministic Auditing**: Full audit trails are a natural consequence of the storage engine, requiring zero application-level logging code.
2. **Crash Resilience**: Because writes are purely appended to the end of the file, recovery is simple: seek to the end of the last complete record and truncate any partial write.
3. **Time-Travel Queries**: You can query the database "as of" any prior point in time (`AS OF`, `SNAPSHOT`) or inspect changes over an interval (`BETWEEN`, `EVOLUTION`, `COMPARE`).
