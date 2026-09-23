# MemoraDB — Full Verification Test Script

Run each section in order (same REPL session, so temporal history builds up
correctly). MemoraDB has **no comment syntax**, so don't paste the `##`
headings into the shell — just the SQL blocks. Everything below is real,
parseable SQL; nothing is placeholder.

If `semantic_test_final` already exists from an earlier run:
```sql
DROP TABLE semantic_test_final;
```

---

## 1. Table setup + SHOW TABLES fix check

```sql
SHOW TABLES;

CREATE TABLE semantic_test_final (id INT PRIMARY KEY, content STRING(200) SEMANTIC);

SHOW TABLES;

DESCRIBE TABLE semantic_test_final;
```
**Expect:** the second `SHOW TABLES` actually prints `semantic_test_final`
(not just a generic "Showed tables" line — that was bug #1).

---

## 2. Insert 5 rows

```sql
INSERT INTO semantic_test_final VALUES (1, "A computer is a machine used for computing and processing information.");
INSERT INTO semantic_test_final VALUES (2, "A database stores and manages structured information.");
INSERT INTO semantic_test_final VALUES (3, "The CPU executes instructions and performs calculations.");
INSERT INTO semantic_test_final VALUES (4, "Operating systems manage computer hardware and software resources.");
INSERT INTO semantic_test_final VALUES (5, "Machine learning allows computers to learn patterns from data.");
```

---

## 3. Basic SELECT + comparison operators

```sql
SELECT * FROM semantic_test_final;

SELECT id, content FROM semantic_test_final;

SELECT * FROM semantic_test_final WHERE id = 1;
SELECT * FROM semantic_test_final WHERE id != 1;
SELECT * FROM semantic_test_final WHERE id < 3;
SELECT * FROM semantic_test_final WHERE id <= 3;
SELECT * FROM semantic_test_final WHERE id > 3;
SELECT * FROM semantic_test_final WHERE id >= 3;

SELECT * FROM semantic_test_final ORDER BY id DESC;
SELECT * FROM semantic_test_final ORDER BY id ASC LIMIT 2;
```

---

## 4. Semantic search (SIMILAR TO)

```sql
SELECT * FROM semantic_test_final WHERE content SIMILAR TO "Computer" LIMIT 5;
SELECT * FROM semantic_test_final WHERE content SIMILAR TO "machine used for computing" LIMIT 5;
SELECT * FROM semantic_test_final WHERE content SIMILAR TO "database information" LIMIT 5;
```

---

## 5. UPDATE, then re-verify + re-search

```sql
UPDATE semantic_test_final SET content = "A computer is an electronic machine used to process and store information." WHERE id = 1;

SELECT * FROM semantic_test_final WHERE id = 1;

SELECT * FROM semantic_test_final WHERE content SIMILAR TO "electronic machine processing information" LIMIT 5;
```

---

## 6. Semantic + temporal combo — both orders (bug #2 fix)

```sql
SELECT * FROM semantic_test_final AS OF 2030-12-31 WHERE content SIMILAR TO "computer" LIMIT 5;

SELECT * FROM semantic_test_final WHERE content SIMILAR TO "computer" AS OF 2030-12-31 LIMIT 5;
```
**Expect:** both return the **same** rows/order. Previously only the first
form parsed at all — the second (`WHERE ... SIMILAR TO ... AS OF ...`) used
to throw a parse error. This is the exact bug you hit.

---

## 7. Timestamp literal granularities (bug #3 — millisecond + partial time)

### 7a. Pairwise defaulting proof — the actual thing to verify

This is the real test: each pair below should resolve to the **exact same
instant**, because the short form is defined to auto-fill whatever's missing
with `00`. Run each pair back-to-back and diff the output — they must be
byte-for-byte identical (same rows, same order, same count).

```sql
-- HH alone  ==  HH:00  (minute auto-00)
SELECT * FROM semantic_test_final AS OF 2030-12-31 23;
SELECT * FROM semantic_test_final AS OF 2030-12-31 23:00;

-- HH:MM alone  ==  HH:MM:00  (second auto-00)
SELECT * FROM semantic_test_final AS OF 2030-12-31 23:59;
SELECT * FROM semantic_test_final AS OF 2030-12-31 23:59:00;

-- HH:MM:SS alone  ==  HH:MM:SS.000  (millisecond auto-00)
SELECT * FROM semantic_test_final AS OF 2030-12-31 23:59:59;
SELECT * FROM semantic_test_final AS OF 2030-12-31 23:59:59.000;

-- fully spelled out midnight, four ways — all four must match each other
SELECT * FROM semantic_test_final AS OF 2030-12-31 00;
SELECT * FROM semantic_test_final AS OF 2030-12-31 00:00;
SELECT * FROM semantic_test_final AS OF 2030-12-31 00:00:00;
SELECT * FROM semantic_test_final AS OF 2030-12-31 00:00:00.000;
```

**One important exception to flag explicitly:** a **bare date with no time
part at all** (`2030-12-31`, nothing after it) is *not* the same as
`2030-12-31 00:00:00.000`. This is existing, intentional MemoraDB behavior
(kept as-is, not something I changed) — a bare date in `AS OF`/`SNAPSHOT`
means "as of the **end** of that whole day" (23:59:59.999-equivalent), while
an explicit `00:00:00.000` means the **exact first instant** of that day.
So:
```sql
SELECT * FROM semantic_test_final AS OF 2030-12-31;             -- whole-day-end semantics
SELECT * FROM semantic_test_final AS OF 2030-12-31 00:00:00.000; -- exact midnight instant
```
Both will *look* the same right now (since all your rows already exist
well before 2030), but they're computing different underlying millisecond
values — worth knowing so it doesn't confuse you later when testing with
dates close to actual insert times.

### 7b. Range-boundary sanity check

Same idea via `BETWEEN`, varying precision only on the *start* boundary — all
five should return identical results since 2020 is far before any real data:

```sql
SELECT * FROM semantic_test_final BETWEEN 2020-01-01 AND 2030-12-31;
SELECT * FROM semantic_test_final BETWEEN 2020-01-01 10 AND 2030-12-31;
SELECT * FROM semantic_test_final BETWEEN 2020-01-01 10:15 AND 2030-12-31;
SELECT * FROM semantic_test_final BETWEEN 2020-01-01 10:15:30 AND 2030-12-31;
SELECT * FROM semantic_test_final BETWEEN 2020-01-01 10:15:30.250 AND 2030-12-31;
```

---

## 8. HISTORY / EVOLUTION / COMPARE

(Using a wide 2020–2030 range instead of a narrow one so it's guaranteed to
catch everything regardless of the date you actually run this on.)

```sql
HISTORY semantic_test_final WHERE id = 1;

EVOLUTION semantic_test_final WHERE id = 1 BETWEEN 2020-01-01 AND 2030-12-31;

COMPARE semantic_test_final WHERE id = 1 BETWEEN 2020-01-01 AND 2030-12-31;
```

---

## 9. Another UPDATE + HISTORY

```sql
UPDATE semantic_test_final SET content = "Computers process information using hardware and software." WHERE id = 3;

SELECT * FROM semantic_test_final WHERE id = 3;

HISTORY semantic_test_final WHERE id = 3;
```

---

## 10. DELETE + verify

```sql
DELETE FROM semantic_test_final WHERE id = 5;

SELECT * FROM semantic_test_final;

HISTORY semantic_test_final WHERE id = 5;
```
**Expect:** the plain `SELECT *` now shows 4 rows; `HISTORY` for id=5 still
shows its past version(s) including the delete.

---

## 11. (Optional / destructive) ROLLBACK + COMPACT syntax

Only run these if you're fine mutating history further — skip otherwise.

```sql
-- row-level rollback: undoes changes to id=1 as of a point in time
ROLLBACK semantic_test_final WHERE id = 1 TO 2020-01-01;

-- whole-table rollback
ROLLBACK TABLE semantic_test_final TO 2020-01-01;

-- purge version history before a date
COMPACT TABLE semantic_test_final TO 2020-01-01;
```
Rolling back `TO 2020-01-01` (before any row existed) is a good edge-case
check — expect either an explicit error/no-op message, not a crash.

---

## 12. Cleanup

```sql
DROP TABLE semantic_test_final;

SHOW TABLES;
```
**Expect:** last `SHOW TABLES` prints `No tables found.` (or lists whatever
other tables you still have, minus this one).

---

### What to send back
Paste the raw REPL output for sections 1, 6, 7, and 10 at minimum — those
are the three actually-changed behaviors. The rest (2–5, 8–9, 11–12) are
existing MemoraDB features included for full coverage, not things I changed.
