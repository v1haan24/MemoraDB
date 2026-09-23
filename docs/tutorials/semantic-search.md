# Tutorial: Semantic Search

In this tutorial, you will explore MemoraDB's native neural vector search capabilities by creating a knowledge base, inserting technical documentation, and searching for concepts using natural language queries.

---

## 1. Create a Semantic Table

Launch `memora` and create a table with a `SEMANTIC` column:

```sql
CREATE TABLE documents (
    id INT PRIMARY KEY,
    category STRING(30),
    content STRING(400) SEMANTIC
);
```

**Output:**
```text
Created table 'documents' (3 columns) with semantic index
```

---

## 2. Populate the Knowledge Base

Insert five technical summaries into the table:

```sql
INSERT INTO documents VALUES (1, "Storage", "A relational database uses structured tables, primary keys, and transaction logs.");
INSERT INTO documents VALUES (2, "AI", "Neural networks and machine learning models discover latent patterns in large datasets.");
INSERT INTO documents VALUES (3, "Hardware", "The central processing unit executes binary instructions and manages hardware registers.");
INSERT INTO documents VALUES (4, "Cloud", "Container clusters enable scalable microservice orchestration and automated failover.");
INSERT INTO documents VALUES (5, "Security", "Public key cryptography ensures secure communication over untrusted networks.");
```

During each insert, MemoraDB's in-process ONNX model automatically computes a 384-dimensional vector embedding for the `content` column and writes it to `data/documents/documents.vec`.

---

## 3. Query Concepts Without Exact Keywords

Let's test semantic similarity by searching for terms that do **not** literally appear in the records:

### Query 1: "Deep learning algorithms"

```sql
SELECT * FROM documents WHERE content SIMILAR TO "Deep learning algorithms" LIMIT 3;
```

**Output Structure:**
```text
-----------------------------------------------------------------------------------------------------------------------------
pk    timestamp                score    content
-----------------------------------------------------------------------------------------------------------------------------
2     [Runtime Timestamp]      [Score]  Neural networks and machine learning models discover latent patterns in large datasets.
5     [Runtime Timestamp]      [Score]  Public key cryptography ensures secure communication over untrusted networks.
1     [Runtime Timestamp]      [Score]  A relational database uses structured tables, primary keys, and transaction logs.
-----------------------------------------------------------------------------------------------------------------------------
3 result(s)
```

*(Note: The score column displays the runtime-computed cosine similarity between your query embedding and each stored row embedding).*

Record `2` is ranked first because its vector representation closely aligns with "deep learning" in the semantic embedding space.

---

### Query 2: "Deploying Docker in Kubernetes"

```sql
SELECT * FROM documents WHERE content SIMILAR TO "Deploying Docker in Kubernetes" LIMIT 3;
```

**Output Structure:**
```text
-----------------------------------------------------------------------------------------------------------------------------
pk    timestamp                score    content
-----------------------------------------------------------------------------------------------------------------------------
4     [Runtime Timestamp]      [Score]  Container clusters enable scalable microservice orchestration and automated failover.
1     [Runtime Timestamp]      [Score]  A relational database uses structured tables, primary keys, and transaction logs.
3     [Runtime Timestamp]      [Score]  The central processing unit executes binary instructions and manages hardware registers.
-----------------------------------------------------------------------------------------------------------------------------
3 result(s)
```

Record `4` is ranked first because the neural model projects concepts of containerization, clusters, and orchestration into the same neighborhood in the 384-dimensional vector space.

---

## 4. Re-Embedding on UPDATE

When you update a record containing a `SEMANTIC` column, MemoraDB generates a fresh embedding in real time:

```sql
UPDATE documents SET content = "Deep learning and convolutional neural networks revolutionized computer vision." WHERE id = 2;
```

Now re-run the search for "computer vision":

```sql
SELECT * FROM documents WHERE content SIMILAR TO "computer vision" LIMIT 2;
```

Record `2` will be returned at the top of the ranked results reflecting its new content!
