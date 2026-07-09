# Current Binary File Format

```
+------------------------------+
| metadataSize      (4 bytes)  |
+------------------------------+
| tableName        (30 bytes)  |
+------------------------------+
| rowCount          (4 bytes)  |
+------------------------------+
| rowSize           (4 bytes)  |
+------------------------------+
| columnCount       (4 bytes)  |
+------------------------------+

For each column:

+------------------------------+
| columnName      (30 bytes)   |
| DataType         (4 bytes)   |
| size             (4 bytes)   |
| offset           (4 bytes)   |
| isPrimaryKey     (1 byte)    |
+------------------------------+

--------------------------------
Records begin here...
```