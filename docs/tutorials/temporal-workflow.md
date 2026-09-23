# Tutorial: Temporal Workflow

In this tutorial, you will explore MemoraDB's temporal database features by tracking an employee's career progression, auditing salary changes, and rolling back state without data loss.

---

## 1. Create the Employees Table

```sql
CREATE TABLE employees (
    id INT PRIMARY KEY,
    name STRING(40),
    title STRING(40),
    salary FLOAT
);
```

---

## 2. Record Career Progression (Simulating Time)

Let's insert an initial employee record:

```sql
INSERT INTO employees VALUES (1, "Elena Rostova", "Junior Engineer", 75000.0);
```

After six months, Elena receives a promotion to Engineer:

```sql
UPDATE employees SET title = "Software Engineer", salary = 95000.0 WHERE id = 1;
```

A year later, Elena is promoted to Senior Engineer:

```sql
UPDATE employees SET title = "Senior Engineer", salary = 125000.0 WHERE id = 1;
```

---

## 3. View the Complete Historical Timeline

Inspect every version in Elena's version chain:

```sql
HISTORY employees WHERE id = 1;
```

**Output Structure:**
```text
---------------------------------------------------------------------------------------------------------
Timestamp                Deleted  id   name           title              salary
---------------------------------------------------------------------------------------------------------
[Runtime Timestamp 1]    NO       1    Elena Rostova  Junior Engineer    75000.00
[Runtime Timestamp 2]    NO       1    Elena Rostova  Software Engineer  95000.00
[Runtime Timestamp 3]    NO       1    Elena Rostova  Senior Engineer    125000.00
---------------------------------------------------------------------------------------------------------
3 version(s) across 1 row(s)
```

*(Note: Timestamps reflect the actual system time when each record was inserted or updated).*

---

## 4. Query Point-in-Time State with `AS OF`

Query Elena's status as of an earlier point in time:

```sql
SELECT title, salary FROM employees AS OF 2026-06-01 WHERE id = 1;
```

*(Note: If the date specified is prior to the first update, the original "Junior Engineer" record is returned).*

---

## 5. Trace Step-by-Step Changes with `EVOLUTION`

Track each specific attribute change across her entire tenure:

```sql
EVOLUTION employees WHERE id = 1 BETWEEN 2026-01-01 AND 2026-12-31;
```

**Output Structure:**
```text
---------------------------------------------------------------------------------------------------------
Timestamp                Column               Before                    After
---------------------------------------------------------------------------------------------------------
[Runtime Timestamp 2]    title                Junior Engineer           Software Engineer
[Runtime Timestamp 2]    salary               75000.00                  95000.00
[Runtime Timestamp 3]    title                Software Engineer         Senior Engineer
[Runtime Timestamp 3]    salary               95000.00                  125000.00
---------------------------------------------------------------------------------------------------------
4 change(s) across 1 row(s)
```

---

## 6. View Net Changes with `COMPARE`

What was the overall net change from when she joined until now?

```sql
COMPARE employees WHERE id = 1 BETWEEN 2026-01-01 AND 2026-12-31;
```

**Output Structure:**
```text
---------------------------------------------------------------------------------------------------------
Column               Before                    After
---------------------------------------------------------------------------------------------------------
title                Junior Engineer           Senior Engineer
salary               75000.00                  125000.00
---------------------------------------------------------------------------------------------------------
2 difference(s) across 1 row(s)
```

`COMPARE` bypasses the intermediate "Software Engineer" stage and displays the net delta directly.

---

## 7. Non-Destructive Rollback

Suppose an administrative error occurred, and we need to roll Elena's compensation back to an earlier state:

```sql
ROLLBACK employees WHERE id = 1 TO 2026-06-01;
```

**Expected Output:**
```text
1 row(s) rolled back
```

Now query the active record:

```sql
SELECT title, salary FROM employees WHERE id = 1;
```

Finally, inspect `HISTORY employees WHERE id = 1;`. You will see **4 versions**! Version 4 contains the restored values, proving that no history was deleted or lost during the rollback.
