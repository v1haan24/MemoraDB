# MemoraDB — Command Reference

All statements are case-insensitive keywords, must end with `;`, and can span
multiple lines. Strings can use `'single'` or `"double"` quotes.

---

## REPL meta-commands (not SQL, no `;` needed)

| Command | Description |
|---|---|
| `.help` | show help |
| `.help <cmd>` | show project-specific syntax for a command or topic, e.g. `.help SELECT`, `.help CREATE TABLE`, `.help WHERE`, `.help DATE` |
| `.tokens <stmt>` | show lexer tokens for a statement |
| `.history` | show command history |
| `.clear` | clear screen |
| `.about` | about MemoraDB |
| `.exit` / `.quit` / `.q` | exit shell |

---

## Data types (for `CREATE TABLE`)

| Type | Notes |
|---|---|
| `INT` | |
| `FLOAT` | |
| `STRING(n)` | size `n` required, e.g. `STRING(50)` |
| `BOOL` | |

Column modifiers (order-independent, each usable once per column):
- `PRIMARY KEY` — exactly one required per table
- `SEMANTIC` — only valid on `STRING` columns; enables `SIMILAR TO` search on that column

---

## DDL

```sql
CREATE TABLE <table> (
    <col1> <TYPE>[(size)] [PRIMARY KEY] [SEMANTIC],
    <col2> <TYPE>[(size)] [PRIMARY KEY] [SEMANTIC],
    ...
);

DROP TABLE <table>;

DESCRIBE TABLE <table>;

SHOW TABLES;
```

Example:
```sql
CREATE TABLE notes (
    id INT PRIMARY KEY,
    title STRING(50),
    body STRING(500) SEMANTIC
);
```

---

## DML

```sql
INSERT INTO <table> VALUES (<v1>, <v2>, ...);

UPDATE <table> SET <col1> = <v1>[, <col2> = <v2> ...] [WHERE <condition>];

DELETE FROM <table> [WHERE <condition>];
```

- `INSERT` takes positional values for every column, in column-definition
  order — no `WHERE` (an INSERT always creates a new row unconditionally).
- `UPDATE` / `DELETE` without `WHERE` apply to **every** row.
- The primary-key column cannot be `UPDATE`d (delete + re-insert instead).

---

## SELECT

```sql
SELECT * | <col1>[, <col2> ...]
FROM <table>
[ <temporal-clause> ]      -- AS OF / BETWEEN ... AND ... / SNAPSHOT
[ WHERE <condition> ]      -- plain comparison OR semantic SIMILAR TO
[ ORDER BY <col> [ASC|DESC] ]
[ LIMIT <n> ];
```

- The temporal clause and `WHERE` are each optional and independent — **either
  order works**: `... AS OF <date> WHERE ...` and `... WHERE ... AS OF <date>`
  both parse fine. (This was just fixed — previously only temporal-before-WHERE
  worked, which is likely what you were hitting.)
- Only one temporal clause and one `WHERE` are allowed per statement (no
  duplicates, no `AND`-chaining of multiple conditions).
- Without a temporal clause, `SELECT` reads the **latest** (current) version
  of each row.

### Temporal clause forms
```sql
... AS OF <date>                          -- snapshot at end of that date/instant
... SNAPSHOT <date>                       -- same as AS OF
... BETWEEN <date1> AND <date2>           -- all versions in the range
```

### WHERE — plain comparison
```sql
WHERE <col> = <value>
WHERE <col> != <value>
WHERE <col> < <value>
WHERE <col> <= <value>
WHERE <col> > <value>
WHERE <col> >= <value>
```
`<value>` is an `INT`, `FLOAT`, or `STRING` literal (unary `-` allowed for numbers).

### WHERE — semantic search
```sql
WHERE <semantic_col> SIMILAR TO "<query text>"
```
- `<semantic_col>` must be declared `SEMANTIC` in `CREATE TABLE`.
- Combine freely with a temporal clause and/or `LIMIT` (defaults to top 10
  results if `LIMIT` is omitted) — see examples below.

### Semantic + temporal combo examples (now working, either order)
```sql
SELECT * FROM notes AS OF 2024-06-01 WHERE body SIMILAR TO "deployment failure";

SELECT * FROM notes WHERE body SIMILAR TO "deployment failure" AS OF 2024-06-01;

SELECT * FROM notes
BETWEEN 2024-01-01 AND 2024-06-01
WHERE body SIMILAR TO "customer complaint"
LIMIT 5;

SELECT * FROM notes
WHERE body SIMILAR TO "customer complaint"
SNAPSHOT 2024-03-15 09:30
LIMIT 5;
```

---

## Temporal-only statements

```sql
COMPARE <table> WHERE <condition> BETWEEN <date1> AND <date2>;

EVOLUTION <table> WHERE <condition> BETWEEN <date1> AND <date2>;

HISTORY <table> WHERE <condition>;

ROLLBACK <table> WHERE <condition> TO <date>;      -- row-level rollback
ROLLBACK TABLE <table> TO <date>;                  -- whole-table rollback

COMPACT TABLE <table> TO <date>;
```
- `COMPARE`, `EVOLUTION`, and `HISTORY` all *require* `WHERE` (no bare-table
  form).
- `ROLLBACK` has two forms: row-level (`WHERE ... TO <date>`) and whole-table
  (`ROLLBACK TABLE <table> TO <date>`).

---

## Date / timestamp literal grammar

```
YYYY-MM-DD
YYYY-MM-DD HH
YYYY-MM-DD HH:MM
YYYY-MM-DD HH:MM:SS
YYYY-MM-DD HH:MM:SS.mmm
```

Defaulting rules (whatever you don't specify is assumed `00`):

| You write | hour | min | sec | ms |
|---|---|---|---|---|
| `2024-06-01` | 00 | 00 | 00 | 00 |
| `2024-06-01 14` | 14 | 00 | 00 | 00 |
| `2024-06-01 14:30` | 14 | 30 | 00 | 00 |
| `2024-06-01 14:30:45` | 14 | 30 | 45 | 00 |
| `2024-06-01 14:30:45.250` | 14 | 30 | 45 | 250 |

Ranges: hour 0–23, minute/second 0–59, millisecond 0–999 (fraction is
truncated to 3 digits, e.g. `.5` → `500`, `.05` → `050`, `.1234` → `123`).

Note: a **date-only** literal (no time part) used in `AS OF` / `SNAPSHOT`
still means "end of that whole day" for range purposes (not just `00:00:00.000`)
— that's existing, intentional behavior for date-only temporal queries, kept
as-is.

---

## Quick cheat-sheet of every statement keyword

`CREATE TABLE` · `DROP TABLE` · `DESCRIBE TABLE` · `SHOW TABLES` ·
`INSERT INTO ... VALUES` · `UPDATE ... SET ... [WHERE]` ·
`DELETE FROM ... [WHERE]` ·
`SELECT ... FROM ... [AS OF|BETWEEN|SNAPSHOT] [WHERE] [ORDER BY] [LIMIT]` ·
`COMPARE ... WHERE ... BETWEEN ... AND ...` ·
`EVOLUTION ... WHERE ... BETWEEN ... AND ...` ·
`HISTORY ... WHERE ...` ·
`ROLLBACK ... WHERE ... TO ...` / `ROLLBACK TABLE ... TO ...` ·
`COMPACT TABLE ... TO ...`
