#include "executor.h"
#include "../vector/vector_meta.h"
#include <cstring>

static bool tokenTypeToDataType(TokenType t, DataType& out) {
    switch (t) {
        case TokenType::INT:    out = DataType::INT;    return true;
        case TokenType::FLOAT:  out = DataType::FLOAT;  return true;
        case TokenType::STRING: out = DataType::STRING; return true;
        case TokenType::BOOL:   out = DataType::BOOL;   return true;
        default: return false;
    }
}
ExecResult Executor::run(const CreateTableStmt& s) {
    TableMeta meta;
    std::strncpy(meta.name, s.tableName.c_str(), tns - 1);
    meta.name[tns - 1] = '\0';
    bool anySemantic = false;
    for (const ColumnDef& c : s.columns) {
        DataType dt;
        if (!tokenTypeToDataType(c.type, dt)) {
            return ExecResult::Error("Unsupported column type on '" + c.name + "'");
        }
        if (dt == DataType::STRING && c.size <= 0) {
            return ExecResult::Error("STRING column '" + c.name +
                                     "' needs a size, e.g. STRING(50)");
        }
        ColMeta col(c.name, dt, c.isPrimaryKey, c.size);
        col.isSemantic = c.isSemantic;
        if (c.isSemantic) {
            if (dt != DataType::STRING) {
                return ExecResult::Error("SEMANTIC is only valid on STRING columns "
                                         "(column '" + c.name + "')");
            }
            anySemantic = true;
        }
        meta.columns.push_back(col);
    }
    if (!catalog.createTable(meta)) {
        return ExecResult::Error("Failed to create table '" + s.tableName + "'");
    }
    if (anySemantic) {
        vecMeta vm;
        VectorMeta vec;
        if (!vm.createVecTable(vec, meta)) {
            return ExecResult::Error("Table '" + s.tableName +
                                     "' was created, but its vector table could not be");
        }
    }
    return ExecResult::Ok("Created table '" + s.tableName + "' (" +
                          std::to_string(meta.columns.size()) + " columns)" +
                          (anySemantic ? " with semantic index" : ""));
}
ExecResult Executor::run(const DropTableStmt& s) {
    if (!catalog.getTable(s.tableName)) {
        return ExecResult::Error("No such table: '" + s.tableName + "'");
    }
    return ExecResult::Error(
        "DROP TABLE is parsed but not executable yet: Catalog has no dropTable() "
        "and holds an open file handle per table, so '" + s.tableName +
        "' can't be removed safely from here. Adding Catalog::dropTable() "
        "(close handle, erase from map, remove_all the directory) would wire this up.");
}
ExecResult Executor::run(const InsertStmt& s) {
    ExecResult err;
    Table* table = requireTable(s.tableName, err);
    if (!table) return err;

    const TableMeta& meta = table->getMeta();
    if (static_cast<int>(s.values.size()) != meta.columnCount) {
        return ExecResult::Error("Table '" + s.tableName + "' has " +
                                 std::to_string(meta.columnCount) + " columns but " +
                                 std::to_string(s.values.size()) + " values were given");
    }
    Row row;
    row.values.reserve(s.values.size());
    for (const Value& v : s.values) row.values.push_back(valueToString(v));
    if (!table->insert(row)) {
        return ExecResult::Error("Insert into '" + s.tableName + "' failed");
    }
    return ExecResult::Ok("1 row inserted");
}
ExecResult Executor::run(const UpdateStmt& s) {
    ExecResult err;
    Table* table = requireTable(s.tableName, err);
    if (!table) return err;

    const TableMeta& meta = table->getMeta();
    int pkCol = primaryKeyColumn(meta);
    if (pkCol < 0) return ExecResult::Error("Table '" + s.tableName + "' has no primary key");
    std::vector<std::pair<int, std::string>> sets;
    for (const Assignment& a : s.assignments) {
        int col = findColumn(meta, a.column);
        if (col < 0) {
            return ExecResult::Error("Unknown column '" + a.column + "' on table '" +
                                     s.tableName + "'");
        }
        if (col == pkCol) {
            return ExecResult::Error("Cannot UPDATE the primary key column '" + a.column +
                                     "' (delete and re-insert instead)");
        }
        sets.emplace_back(col, valueToString(a.value));
    }
    std::vector<Record> targets = table->scanLatest();
    if (s.where) {
        WhereClause clause;
        if (!buildWhere(meta, *s.where, clause, err)) return err;
        targets = where(targets, meta, clause);
    }
    int updated = 0;
    for (const Record& rec : targets) {
        Row row = rec.row;
        for (const auto& kv : sets) row.values[kv.first] = kv.second;
        if (!table->update(row)) {
            return ExecResult::Error("Update failed on row with primary key '" +
                                     rec.row.values[pkCol] + "' (" +
                                     std::to_string(updated) + " row(s) already updated)");
        }
        ++updated;
    }
    return ExecResult::Ok(std::to_string(updated) + " row(s) updated");
}
ExecResult Executor::run(const DeleteStmt& s) {
    ExecResult err;
    Table* table = requireTable(s.tableName, err);
    if (!table) return err;
    const TableMeta& meta = table->getMeta();
    int pkCol = primaryKeyColumn(meta);
    if (pkCol < 0) return ExecResult::Error("Table '" + s.tableName + "' has no primary key");
    std::vector<Record> targets = table->scanLatest();
    if (s.where) {
        WhereClause clause;
        if (!buildWhere(meta, *s.where, clause, err)) return err;
        targets = where(targets, meta, clause);
    }
    int deleted = 0;
    for (const Record& rec : targets) {
        if (!table->deleteRow(rec.row.values[pkCol])) {
            return ExecResult::Error("Delete failed on row with primary key '" +
                                     rec.row.values[pkCol] + "' (" +
                                     std::to_string(deleted) + " row(s) already deleted)");
        }
        ++deleted;
    }
    return ExecResult::Ok(std::to_string(deleted) + " row(s) deleted");
}
