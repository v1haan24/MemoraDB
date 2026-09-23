# Tutorial: Student Database

In this beginner-friendly tutorial, you will create a student records database, insert student profiles, filter and sort query results, perform updates, and delete records.

---

## 1. Create the Table

Launch `memora` and create a `students` table:

```sql
CREATE TABLE students (
    student_id INT PRIMARY KEY,
    name STRING(50),
    major STRING(40),
    gpa FLOAT,
    is_active BOOL
);
```

**Output:**
```text
Created table 'students' (5 columns)
```

Verify the schema:

```sql
DESCRIBE TABLE students;
```

---

## 2. Insert Records

Insert four students into the table using positional column order:

```sql
INSERT INTO students VALUES (101, "Alice Chen", "Computer Science", 3.85, true);
INSERT INTO students VALUES (102, "Bob Martin", "Mechanical Engineering", 3.40, true);
INSERT INTO students VALUES (103, "Clara Gomez", "Computer Science", 3.92, true);
INSERT INTO students VALUES (104, "David Lee", "Mathematics", 2.95, false);
```

---

## 3. Query All Students

Retrieve the entire table:

```sql
SELECT * FROM students;
```

Project only the student names and GPAs:

```sql
SELECT name, gpa FROM students;
```

---

## 4. Filter with WHERE

Find all Computer Science majors:

```sql
SELECT student_id, name, gpa FROM students WHERE major = "Computer Science";
```

Find students on the Dean's List (GPA >= 3.8):

```sql
SELECT name, gpa FROM students WHERE gpa >= 3.8;
```

---

## 5. Sort with ORDER BY and LIMIT

Rank the top 2 active students by GPA:

```sql
SELECT name, major, gpa FROM students ORDER BY gpa DESC LIMIT 2;
```

**Output:**
```text
Clara Gomez  Computer Science  3.92
Alice Chen   Computer Science  3.85
```

---

## 6. Update a Record

Suppose Bob Martin improves his GPA to `3.65`:

```sql
UPDATE students SET gpa = 3.65 WHERE student_id = 102;
```

**Output:**
```text
1 row(s) updated
```

Verify that the updated GPA is reflected:

```sql
SELECT * FROM students WHERE student_id = 102;
```

---

## 7. Delete a Record

Delete the inactive student record:

```sql
DELETE FROM students WHERE student_id = 104;
```

**Output:**
```text
1 row(s) deleted
```

Verify that David Lee no longer appears in the active student list:

```sql
SELECT * FROM students;
```

---

## 8. Historical Verification

Even though David Lee was deleted, his history is safely preserved in MemoraDB's append-only storage! You can inspect his past record at any time:

```sql
HISTORY students WHERE student_id = 104;
```
