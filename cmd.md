# MemoraDB Command Reference (v0.1.0-dev)

Auto-extracted from `src/lexer` + `src/parser` grammar (not hand-typed).

Format:
```
command   : <name>
syntax    : <syntax>
example   : <example>
```

**Notes / gotchas**
- Keywords are case-insensitive (`CREATE` == `create` == `Create`).
- Every statement must end with a semicolon `;`.
- Dates are literal `YYYY-MM-DD` — **date only, no time-of-day**. `DateLiteral` in
  `ast.h` only has `year/month/day` fields; there's no `HH:MM:SS` support in the
  parser, so you can't do `AS OF 2026-01-15 14:30`. (Internally each row version
  *is* stamped with a millisecond timestamp via `chrono::system_clock` in
  `storage/serialization.cpp` — the engine has the precision, the query language
  just doesn't expose it. Adding time-of-day would mean extending `DateLiteral`.)
- Strings use double quotes `"..."`.
- `WHERE` supports exactly **one** condition (`col OP value`) — no `AND`/`OR`
  chaining inside `WHERE`. Comparison ops: `= != < <= > >=`
- **Semantic + temporal combos are syntactically legal.** `parseSelect()` parses
  the temporal clause (`AS OF`/`BETWEEN`/`SNAPSHOT`) *before* `WHERE`, and `WHERE`
  can hold a `SIMILAR TO` condition — so e.g.
  `SELECT * FROM users AS OF 2026-01-15 WHERE bio SIMILAR TO "ml student";`
  parses fine. It just won't *run* yet, because `SIMILAR TO` is parsed but the
  executor still rejects it (vector layer not wired as of v0.1.0-dev).

## DDL — Data Definition

command   : CREATE TABLE
syntax    : CREATE TABLE <table_name> ( <col_name> <TYPE> [(<size>)] [PRIMARY KEY] [SEMANTIC], ... );
example   : CREATE TABLE users (id INT PRIMARY KEY, name STRING(50), bio STRING(500) SEMANTIC, gpa FLOAT, active BOOL);

command   : DROP TABLE
syntax    : DROP TABLE <table_name>;
example   : DROP TABLE users;

command   : DESCRIBE TABLE
syntax    : DESCRIBE TABLE <table_name>;
example   : DESCRIBE TABLE users;

> Column types: `INT` | `FLOAT` | `STRING(<size>)` | `BOOL`
> Column modifiers (any order, each at most once): `PRIMARY KEY`, `SEMANTIC`
> `SEMANTIC` is only meaningful on `STRING` columns (embedded by `ml_worker.py`).

## DML — Data Manipulation

command   : INSERT
syntax    : INSERT INTO <table_name> VALUES ( <value>, <value>, ... );
example   : INSERT INTO users VALUES (1, "Vihaan", "B.Tech IT student at VJTI", 8.7, true);

command   : UPDATE
syntax    : UPDATE <table_name> SET <col> = <value>, ... [WHERE <col> <OP> <value>];
example   : UPDATE users SET gpa = 9.1 WHERE id = 1;

command   : DELETE
syntax    : DELETE FROM <table_name> [WHERE <col> <OP> <value>];
example   : DELETE FROM users WHERE active = false;

## Queries

command   : SELECT (basic)
syntax    : SELECT * | <col>, <col>, ... FROM <table_name> [WHERE <col> <OP> <value>] [ORDER BY <col> [ASC|DESC]] [LIMIT <n>];
example   : SELECT name, gpa FROM users WHERE gpa > 8.0 ORDER BY gpa DESC LIMIT 10;

command   : SELECT (SIMILAR TO — semantic search, not yet executed)
syntax    : SELECT * FROM <table_name> WHERE <semantic_col> SIMILAR TO "<text>";
example   : SELECT * FROM users WHERE bio SIMILAR TO "machine learning student";

command   : SELECT (temporal + semantic combo — parses, does not yet execute)
syntax    : SELECT * FROM <table_name> [AS OF <date> | BETWEEN <date> AND <date> | SNAPSHOT <date>] WHERE <semantic_col> SIMILAR TO "<text>";
example   : SELECT * FROM users AS OF 2026-01-15 WHERE bio SIMILAR TO "ml student";

## Temporal Queries (append-only versioned storage)

command   : SELECT ... AS OF
syntax    : SELECT * FROM <table_name> AS OF <YYYY-MM-DD> [WHERE ...] [ORDER BY ...] [LIMIT ...];
example   : SELECT * FROM users AS OF 2026-01-15;

command   : SELECT ... BETWEEN
syntax    : SELECT * FROM <table_name> BETWEEN <YYYY-MM-DD> AND <YYYY-MM-DD> [WHERE ...] [ORDER BY ...] [LIMIT ...];
example   : SELECT * FROM users BETWEEN 2026-01-01 AND 2026-06-30;

command   : SELECT ... SNAPSHOT
syntax    : SELECT * FROM <table_name> SNAPSHOT <YYYY-MM-DD> [WHERE ...] [ORDER BY ...] [LIMIT ...];
example   : SELECT * FROM users SNAPSHOT 2026-03-01;

command   : HISTORY
syntax    : HISTORY <table_name> WHERE <col> <OP> <value>;
example   : HISTORY users WHERE id = 1;

command   : COMPARE
syntax    : COMPARE <table_name> WHERE <col> <OP> <value> BETWEEN <YYYY-MM-DD> AND <YYYY-MM-DD>;
example   : COMPARE users WHERE id = 1 BETWEEN 2026-01-01 AND 2026-06-01;

command   : EVOLUTION
syntax    : EVOLUTION <table_name> WHERE <col> <OP> <value> BETWEEN <YYYY-MM-DD> AND <YYYY-MM-DD>;
example   : EVOLUTION users WHERE id = 1 BETWEEN 2026-01-01 AND 2026-06-01;

## Rollback / Compaction

command   : ROLLBACK (row-level)
syntax    : ROLLBACK <table_name> WHERE <col> <OP> <value> TO <YYYY-MM-DD>;
example   : ROLLBACK users WHERE id = 1 TO 2026-02-01;

command   : ROLLBACK TABLE (whole table)
syntax    : ROLLBACK TABLE <table_name> TO <YYYY-MM-DD>;
example   : ROLLBACK TABLE users TO 2026-02-01;

command   : COMPACT TABLE
syntax    : COMPACT TABLE <table_name> TO <YYYY-MM-DD>;
example   : COMPACT TABLE users TO 2026-01-01;

## Full Keyword List (`lexer.cpp`)

```
CREATE DROP DESCRIBE TABLE INSERT INTO VALUES UPDATE SET DELETE SELECT FROM
WHERE ORDER BY LIMIT ASC DESC AND PRIMARY KEY INT FLOAT STRING BOOL SEMANTIC
AS OF BETWEEN SNAPSHOT COMPARE EVOLUTION HISTORY ROLLBACK TO COMPACT SIMILAR
```