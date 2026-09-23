# CLI & REPL Reference

The MemoraDB interactive shell (REPL) provides an environment for executing SQL queries, inspecting lexer tokens, reviewing command history, and monitoring database engine status.

---

## Meta-Commands vs. SQL Statements

The shell distinguishes between two classes of input:

* **SQL Statements**: Parsed by the SQL compiler, must terminate with a semicolon (`;`), and operate on database tables.
* **Meta-Commands**: Begin with a dot (`.`), do **not** use a semicolon, and control shell behavior.

---

## Supported Meta-Commands

| Command | Arguments | Description |
|---|---|---|
| `.help` | `[topic]` | Displays general shell help, or detailed syntax for a specific SQL command or concept (e.g., `.help SELECT`, `.help WHERE`, `.help DATE`). |
| `.tokens` | `<statement>` | Passes a statement through the lexer and prints a table of token types, line numbers, column offsets, and token values. |
| `.history` | *(none)* | Prints all SQL statements and meta-commands entered during the current session. |
| `.clear` / `.cls` | *(none)* | Clears the terminal screen. |
| `.about` | *(none)* | Displays the project summary, core architectural features, and official GitHub repository link. |
| `.exit` / `.quit` / `.q` | *(none)* | Closes the database catalog and exits the REPL. |

> [!NOTE]
> Bare `exit` and `quit` (without a leading dot) are also accepted as aliases to exit the shell.

---

## Detailed Command Usage

### 1. `.help [topic]`

Without arguments, `.help` prints a summary of commands. When passed a topic name, it displays detailed syntax and examples:

```text
memora> .help SELECT
SELECT
Syntax:
  SELECT * | <col1>[, <col2> ...]
  FROM <table>
  [AS OF <date> | SNAPSHOT <date> | BETWEEN <date1> AND <date2>]
  [WHERE <condition>]
  [ORDER BY <col> [ASC|DESC]]
  [LIMIT <n>];

Notes:
  WHERE and the temporal clause can appear in either order.
  SIMILAR TO is supported only on SEMANTIC columns.

Examples:
  SELECT * FROM notes;
  SELECT id, body FROM notes AS OF 2026-09-20 WHERE id = 1;
  SELECT * FROM notes WHERE body SIMILAR TO 'database search' LIMIT 5;
```

**Supported Topics**: `CREATE`, `DROP`, `DESCRIBE`, `SHOW`, `INSERT`, `UPDATE`, `DELETE`, `SELECT`, `WHERE`, `SIMILAR`, `AS OF`, `ORDER BY`, `LIMIT`, `COMPARE`, `EVOLUTION`, `HISTORY`, `ROLLBACK`, `COMPACT`, `DATE`, `TYPES`, `CLEAR`.

---

### 2. `.tokens <statement>`

The `.tokens` command exposes the internal lexical analysis pipeline. It tokenizes the input string without executing it and displays each token with syntax coloring:

```text
memora> .tokens SELECT * FROM notes WHERE id = 1;
#    type                line:col  value
0    SELECT              1:1       SELECT
1    STAR                1:8       *
2    FROM                1:10      FROM
3    IDENTIFIER          1:15      notes
4    WHERE               1:21      WHERE
5    IDENTIFIER          1:27      id
6    EQUAL               1:30      =
7    INTEGER_LITERAL     1:32      1
8    SEMICOLON           1:33      ;
9    END_OF_FILE         1:34      
Tokens: 10 (0.142 ms)
```

---

### 3. `.history`

Prints the chronological list of commands executed in the current REPL session:

```text
memora> .history
1  CREATE TABLE notes (id INT PRIMARY KEY, content STRING(200) SEMANTIC);
2  INSERT INTO notes VALUES (1, "MemoraDB temporal test");
3  SELECT * FROM notes;
```

---

## Terminal Features

* **Truecolor & 256-Color Gradients**: The REPL automatically detects 24-bit TrueColor (`COLORTERM=truecolor`) and ANSI-256 terminals, rendering a full-color banner on launch.
* **Interactive Diagnostic LEDs**: Displays real-time status indicators for the temporal engine, append-only storage, vector index, and ONNX model.
* **Console Title**: Sets the terminal emulator window title to `"MemoraDB"`.
* **Signal Handling**: Catches `SIGINT` (Ctrl+C) gracefully to prevent database file corruption during active writes.
