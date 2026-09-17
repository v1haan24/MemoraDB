#include "executor.h"

// ============================================================================
// COMPARE <table> WHERE <cond> BETWEEN <d1> AND <d2>
//
// Table::compare(pk,t1,t2) works one row at a time, so the WHERE is first
// resolved to matching primary keys and the diffs concatenated.
// ============================================================================
ExecResult Executor::run(const CompareStmt& s) {
    ExecResult err;
    Table* table = requireTable(s.tableName, err);
    if (!table) return err;

    uint64_t t1 = dayStartMs(s.rangeStart);
    uint64_t t2 = dayEndMs(s.rangeEnd);
    if (t1 > t2) return ExecResult::Error("COMPARE range starts after it ends");

    std::vector<std::string> pks = resolvePks(*table, s.where, err);
    if (!err.ok()) return err;
    if (pks.empty()) return ExecResult::Ok("No rows matched");

    ExecResult r;
    r.kind = ExecResult::Kind::DIFFS;
    for (const std::string& pk : pks) {
        std::vector<Difference> d = table->compare(pk, t1, t2);
        r.diffs.insert(r.diffs.end(), d.begin(), d.end());
    }
    r.message = std::to_string(r.diffs.size()) + " difference(s) across " +
                std::to_string(pks.size()) + " row(s)";
    return r;
}

// ============================================================================
// EVOLUTION <table> WHERE <cond> BETWEEN <d1> AND <d2>
//
// Same shape as COMPARE, but Table::evolution() walks every intermediate
// version instead of only diffing the endpoints.
// ============================================================================
ExecResult Executor::run(const EvolutionStmt& s) {
    ExecResult err;
    Table* table = requireTable(s.tableName, err);
    if (!table) return err;

    uint64_t t1 = dayStartMs(s.rangeStart);
    uint64_t t2 = dayEndMs(s.rangeEnd);
    if (t1 > t2) return ExecResult::Error("EVOLUTION range starts after it ends");

    std::vector<std::string> pks = resolvePks(*table, s.where, err);
    if (!err.ok()) return err;
    if (pks.empty()) return ExecResult::Ok("No rows matched");

    ExecResult r;
    r.kind = ExecResult::Kind::DIFFS;
    for (const std::string& pk : pks) {
        std::vector<Difference> d = table->evolution(pk, t1, t2);
        r.diffs.insert(r.diffs.end(), d.begin(), d.end());
    }
    r.message = std::to_string(r.diffs.size()) + " change(s) across " +
                std::to_string(pks.size()) + " row(s)";
    return r;
}

// ============================================================================
// HISTORY <table> WHERE <cond>
//
// Returns every stored version of the matching row(s), newest-to-oldest as the
// engine stores them.
// ============================================================================
ExecResult Executor::run(const HistoryStmt& s) {
    ExecResult err;
    Table* table = requireTable(s.tableName, err);
    if (!table) return err;

    std::vector<std::string> pks = resolvePks(*table, s.where, err);
    if (!err.ok()) return err;
    if (pks.empty()) return ExecResult::Ok("No rows matched");

    ExecResult r;
    r.kind = ExecResult::Kind::ROWS;
    r.columns = table->getMeta().columns;
    for (const std::string& pk : pks) {
        std::vector<Record> versions = table->showHistory(pk);
        r.records.insert(r.records.end(), versions.begin(), versions.end());
    }
    r.message = std::to_string(r.records.size()) + " version(s) across " +
                std::to_string(pks.size()) + " row(s)";
    return r;
}

// ============================================================================
// ROLLBACK
//   ROLLBACK TABLE <t> TO <date>          -> whole-table rollback
//   ROLLBACK <t> WHERE <cond> TO <date>   -> per-row rollback
// ============================================================================
ExecResult Executor::run(const RollbackStmt& s) {
    ExecResult err;
    Table* table = requireTable(s.tableName, err);
    if (!table) return err;

    uint64_t ts = dayEndMs(s.toDate);

    if (s.wholeTable) {
        if (!table->rollback(ts)) {
            return ExecResult::Error("Rollback of table '" + s.tableName + "' failed");
        }
        return ExecResult::Ok("Table '" + s.tableName + "' rolled back");
    }

    if (!s.where) {
        return ExecResult::Error("ROLLBACK without TABLE requires a WHERE clause");
    }

    std::vector<std::string> pks = resolvePks(*table, *s.where, err);
    if (!err.ok()) return err;
    if (pks.empty()) return ExecResult::Ok("No rows matched");

    int rolled = 0;
    for (const std::string& pk : pks) {
        if (!table->rollback(pk, ts)) {
            return ExecResult::Error("Rollback failed on row with primary key '" + pk +
                                     "' (" + std::to_string(rolled) + " row(s) already rolled back)");
        }
        ++rolled;
    }
    return ExecResult::Ok(std::to_string(rolled) + " row(s) rolled back");
}

// ============================================================================
// COMPACT TABLE <t> TO <date>
//
// Passes the vector handles through when the table has them, so the main and
// vector compactions run together inside Table::compact() rather than the
// vector side being left stale.
// ============================================================================
ExecResult Executor::run(const CompactStmt& s) {
    ExecResult err;
    Table* table = requireTable(s.tableName, err);
    if (!table) return err;

    uint64_t ts = dayEndMs(s.toDate);
    VectorHandles* vh = vectorsFor(*table);

    bool ok = vh ? table->compact(ts, vh->vt.get(), vh->idx.get())
                 : table->compact(ts);
    if (!ok) return ExecResult::Error("Compaction of '" + s.tableName + "' failed");

    // Compaction renumbers vector ids, so the cached index is stale now.
    if (vh) vh->idx->buildIndex(*vh->vt);

    return ExecResult::Ok("Compacted '" + s.tableName + "'" +
                          (vh ? " (with vector table)" : ""));
}
