/*
Database File Layout

4 bytes   metadataSize
30 bytes  tableName
4 bytes   rowCount
4 bytes   rowSize
4 bytes   columnCount

Repeated for each column:
30 bytes  columnName
4 bytes   DataType
4 bytes   size
4 bytes   offset
1 byte    isPrimaryKey

-------------------------
Records begin here
*/