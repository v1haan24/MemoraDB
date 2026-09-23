# Quick Start Guide

Get up and running with MemoraDB in 5 minutes. This tutorial guides you from launching the REPL to running your first relational, temporal, and semantic queries.

---

## 1. Start the MemoraDB Shell

Launch the executable from your terminal:

```powershell
.\build\memora.exe
```

You are greeted by the interactive prompt:

```text
memora>
```

> [!NOTE]
> All SQL statements in MemoraDB must end with a semicolon (`;`). Statements can span multiple lines. REPL meta-commands begin with a dot (`.`) and do not use semicolons.

---

## 2. Create Your First Table

Let's create a table named `documents` with an integer primary key, a category string, and a semantic text body:

```sql
CREATE TABLE documents (
    id INT PRIMARY KEY,
    category STRING(30),
    content STRING(300) SEMANTIC
);
```

**Expected Output:**
```text
Created table 'documents' (3 columns) with semantic index
```

---

## 3. Insert Initial Records

Insert three records using positional column order:

```sql
INSERT INTO documents VALUES (1, "Databases", "Relational databases use tables, rows, and B-tree indexes.");
INSERT INTO documents VALUES (2, "DevOps", "Kubernetes automates container deployment, scaling, and operations.");
INSERT INTO documents VALUES (3, "Artificial Intelligence", "Large language models generate text based on neural transformer architectures.");
```

**Expected Output:**
```text
1 row inserted
1 row inserted
1 row inserted
```

During each insert into a table with a `SEMANTIC` column, MemoraDB generates a 384-dimensional embedding vector in-process using the MiniLM model and appends it to `data/documents/documents.vec`.

---

## 4. Query Current Data

Retrieve all records:

```sql
SELECT * FROM documents;
```

Filter by an exact match on a column:

```sql
SELECT id, category FROM documents WHERE id = 2;
```

---

## 5. Perform Semantic Vector Search

Search for concepts even if the exact keywords are not present:

```sql
SELECT * FROM documents WHERE content SIMILAR TO "orchestrating docker containers" LIMIT 3;
```

**Result:**
Row `2` ("Kubernetes automates container deployment...") is returned at the top of the results with a high cosine similarity score, despite not containing the word "docker".

---

## 6. Update a Record and Observe Versioning

Update row `1`:

```sql
UPDATE documents SET content = "Relational databases support SQL queries and transactional ACID guarantees." WHERE id = 1;
```

Now inspect the complete historical timeline for row `1`:

```sql
HISTORY documents WHERE id = 1;
```

You will see two versions of row `1`: the original record from step 3 and the updated record from step 6, each marked with its creation timestamp.

---

## 7. Query Past State with `AS OF`

Query how row `1` looked before the update:

```sql
SELECT * FROM documents AS OF 2026-01-01 WHERE id = 1;
```

If the timestamp provided is before the row existed, no record is returned. If you supply the timestamp corresponding to the first insert, the original version is returned!

---

## 8. Useful REPL Commands

Try out the REPL utilities:

* `.tokens SELECT * FROM documents;` — Inspect how the lexer tokenizes your statement.
* `.history` — Display your shell command history.
* `.help SELECT` — Display the precise grammar rules for `SELECT`.
* `.clear` — Clear the terminal screen.
* `.exit` — Exit the MemoraDB shell.
