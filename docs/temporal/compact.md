# COMPACT TABLE

## Description

The `COMPACT TABLE` statement prunes superseded historical versions older than a specified anchor date to reclaim disk space. To prevent accidental data loss, MemoraDB archives the pre-compaction data file before activating the compacted storage.

---

## Syntax

*(See the [Syntax Notation Guide](../reference/grammar.md#syntax-notation-guide) for notation rules).*

### Basic syntax

```sql
COMPACT TABLE <table> TO <date>;
```

### Full syntax

```sql
COMPACT TABLE <table> TO <date>;
```

---

## Parameters

| Parameter | Description |
|---|---|
| `<table>` | Target table name. |
| `TO <date>` | The cutoff date or timestamp. Versions prior to each row's anchor at this date are purged from active storage. |

---

## Example (`employees`)

```sql
COMPACT TABLE employees TO 2026-01-01;
```

**Expected Output:**
```text
Compacted 'employees'
```
*(Or `Compacted 'employees' (with vector table)` if a semantic index exists).*

---

## How It Works

Compaction proceeds in four atomic stages:

### 1. Anchor Identification
For each primary key in the table:
* Finds the latest version committed at or before the cutoff timestamp (`anchor`).
* Retains the anchor version and **all subsequent versions**.
* Versions older than the anchor are marked for exclusion.

### 2. Temporary File Generation
* Opens `data/<table_name>/data.db.tmp`.
* Copies over the table metadata header.
* Sequentially writes each retained record, recording new file offsets into a fresh `HistoryIndex`.
* If a companion vector table exists (`.vec`), `vt->startRewrite()` and `vt->copyRecord()` mirror the process.

### 3. Safe Archival
* Moves the active `data.db` to:
  ```text
  data/<table_name>/archive/archive_<timestamp>.db
  ```
* Activates `data.db.tmp` by renaming it to `data.db`.

### 4. Index Swap
* Replaces the table's in-memory `HistoryIndex` with the newly computed contiguous index.

---

## Safety Guarantees

* **Non-Destructive Archival**: The pre-compaction database file is preserved in the `archive/` folder.
* **Point-in-Time Integrity**: Because the anchor version is retained, running `AS OF <date>` queries at or after the cutoff date remains 100% accurate.
* **Vector Table Synchronization**: Vector embeddings are rewritten and compacted in lockstep with relational records.

---

## Related Commands

* [`ROLLBACK`](rollback.md) — Restore past row versions.
* [`HISTORY`](history.md) — Inspect version counts before and after compaction.
