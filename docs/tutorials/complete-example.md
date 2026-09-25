# Tutorial: Complete Real-World Workflow

This tutorial ties together all of MemoraDB's capabilities—relational constraints, append-only temporal tracking, and in-process neural vector search—by building an **Engineering Incident & Post-Mortem Tracking System**.

---

## The Scenario

As an infrastructure team, you need a database that can:
1. Store production incident tickets.
2. Track how an incident was diagnosed and resolved over time.
3. Allow engineers to travel back in time to inspect exactly what was known at each hour of an outage.
4. Search historical incident post-mortems using natural language to quickly diagnose future issues.

---

## Step 1: Create the Incidents Table

```sql
CREATE TABLE incidents (
    id INT PRIMARY KEY,
    service STRING(30),
    severity STRING(10),
    summary STRING(500) SEMANTIC
);
```

---

## Step 2: Record the Incident Life Cycle

### Initial Alert
```sql
INSERT INTO incidents VALUES (101, "AuthService", "SEV-1", "Users experiencing HTTP 500 errors during OAuth login redirect.");
```

### Root Cause Identified
```sql
UPDATE incidents SET summary = "Root cause identified as database connection pool exhaustion in the user authentication cluster." WHERE id = 101;
```

### Mitigation Applied
```sql
UPDATE incidents SET severity = "SEV-2", summary = "Increased max connection limit to 500. Login latency stabilizing across all regions." WHERE id = 101;
```

### Incident Resolved
```sql
UPDATE incidents SET severity = "SEV-3", summary = "Permanent fix deployed. Connection pool leak resolved in patch v2.4.1. Incident closed." WHERE id = 101;
```

---

## Step 3: Audit Incident Evolution

To understand how the diagnosis progressed during the post-mortem review:

```sql
EVOLUTION incidents WHERE id = 101 BETWEEN 2026-01-01 AND 2026-12-31;
```

**Output Structure:**
```text
----------------------------------------------------------------------------------------------------------------------------------
Timestamp                Column     Before                                                  After
----------------------------------------------------------------------------------------------------------------------------------
[Runtime Timestamp 1]    summary    Users experiencing HTTP 500 errors...                   Root cause identified as database conn...
[Runtime Timestamp 2]    severity   SEV-1                                                   SEV-2
[Runtime Timestamp 2]    summary    Root cause identified as database conn...               Increased max connection limit to 500...
[Runtime Timestamp 3]    severity   SEV-2                                                   SEV-3
[Runtime Timestamp 3]    summary    Increased max connection limit to 500...                Permanent fix deployed. Connection...
----------------------------------------------------------------------------------------------------------------------------------
5 change(s) across 1 row(s)
```

---

## Step 4: Time Travel with `AS OF`

What was the status and summary of the incident when it was first reported, before the root cause was discovered?

```sql
SELECT severity, summary FROM incidents AS OF 2026-01-01 WHERE id = 101;
```

*(Or use the timestamp corresponding to the initial insert).*

---

## Step 5: Semantic Troubleshooting for Future Incidents

Months later, another team experiences login failures and runs a semantic search to check if a similar issue occurred in the past:

```sql
SELECT * FROM incidents 
WHERE summary SIMILAR TO "database out of connections during authentication" 
LIMIT 3;
```

**Output Structure:**
```text
----------------------------------------------------------------------------------------------------------------------------------
pk    timestamp                score    summary
----------------------------------------------------------------------------------------------------------------------------------
101   [Runtime Timestamp 3]    [Score]  Permanent fix deployed. Connection pool leak resolved in patch v2.4.1. Incident closed.
----------------------------------------------------------------------------------------------------------------------------------
1 result(s)
```

The team instantly discovers incident `101` and the resolution patch, cutting MTTR (Mean Time to Resolution) dramatically!

---

## Step 6: Summary of Capabilities Demonstrated

In this complete workflow, you used:
* **Relational Schema**: Enforced types and primary keys on disk.
* **Append-Only Immutability**: Captured every phase of the outage without losing prior observations.
* **Temporal Auditing**: Inspected the timeline using `EVOLUTION` and `AS OF`.
* **Deep Neural Search**: Found historical incidents using natural-language concept matching rather than exact keyword guessing.
