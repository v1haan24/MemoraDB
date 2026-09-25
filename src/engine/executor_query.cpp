#include "executor.h"
#include "../query/temporal_semantic.h"
#include <cstdint>

namespace {

Record exactVersion(Table& table, const std::string& pk, uint64_t timestamp) {
    for (const Record& record : table.showHistory(pk)) {
        if (record.timestamp == timestamp) return record;
    }
    return {};
}

}

ExecResult Executor::run(const SelectStmt& s) {
    ExecResult err;
    Table* table = requireTable(s.tableName, err);
    if (!table) return err;

    const TableMeta& meta = table->getMeta();
    CandidateMode mode = LATEST;
    uint64_t t1 = 0, t2 = 0;
    switch (s.temporalMode) {
        case TemporalMode::NONE:
            mode = LATEST;
            break;
        case TemporalMode::AS_OF:
            mode = SNAPSHOT;
            t1 = dayEndMs(*s.asOfDate);
            break;
        case TemporalMode::SNAPSHOT:
            mode = SNAPSHOT;
            t1 = dayEndMs(*s.snapshotDate);
            break;
        case TemporalMode::BETWEEN:
            mode = BETWEEN;
            t1 = dayStartMs(*s.betweenStart);
            t2 = dayEndMs(*s.betweenEnd);
            if (t1 > t2) {
                return ExecResult::Error("BETWEEN range starts after it ends");
            }
            break;
    }

    // ---- semantic branch ----
    if (s.where && s.where->op == CompareOp::SIMILAR_TO) {
        int32_t col = findColumn(meta, s.where->column);
        if (col < 0) {
            return ExecResult::Error("Unknown column '" + s.where->column +
                                     "' on table '" + s.tableName + "'");
        }
        if (!meta.columns[col].isSemantic) {
            return ExecResult::Error("Column '" + s.where->column +
                                     "' is not declared SEMANTIC, so SIMILAR TO can't be used on it");
        }

        VectorHandles* vh = vectorsFor(*table);
        if (!vh) {
            return ExecResult::Error("No vector table found for '" + s.tableName +
                                     "' -- no semantic index is available");
        }
        if (!embedder) {
            return ExecResult::Error(
                "SIMILAR TO needs the query text embedded, but no embedding provider is "
                "configured. MemoraDB needs the in-process ONNX embedding provider; "
                "supply one with Executor::setEmbeddingProvider() to enable this.");
        }

        float queryEmbedding[VEC_DIM] = {};
        if (!embedder(s.where->value.strVal, queryEmbedding)) {
            return ExecResult::Error("Failed to embed the query text");
        }

        int32_t k = s.limit ? *s.limit : 10;
        ExecResult r;
        r.kind = ExecResult::Kind::SEARCH;
        r.search = temporalSemanticSearch(*table, *vh->idx, mode, t1, t2,
                                          queryEmbedding, k, nullptr);
        std::vector<int32_t> semanticColumns;
        for (uint32_t i = 0; i < meta.columnCount; ++i) {
            if (meta.columns[i].isSemantic) {
                semanticColumns.push_back(i);
                r.columns.push_back(meta.columns[i]);
            }
        }
        for (SearchResult& result : r.search) {
            Record record = exactVersion(*table, result.pk, result.timestamp);
            for (int32_t col : semanticColumns) {
                if (col < static_cast<int32_t>(record.row.values.size())) {
                    result.semanticValues.push_back(record.row.values[col]);
                } else {
                    result.semanticValues.push_back("");
                }
            }
        }
        r.message = std::to_string(r.search.size()) + " result(s)";
        return r;
    }

    // ---- ordinary branch ----
    std::vector<Record> rows = generateCandidates(*table, mode, t1, t2);

    if (s.where) {
        WhereClause clause;
        if (!buildWhere(meta, *s.where, clause, err)) return err;
        rows = where(rows, meta, clause);
    }

    if (s.orderByColumn) {
        int32_t col = findColumn(meta, *s.orderByColumn);
        if (col < 0) {
            return ExecResult::Error("Unknown column '" + *s.orderByColumn + "' in ORDER BY");
        }
        rows = sortRecords(rows, meta, col, !s.orderDescending);
    }

    if (s.limit) rows = limitRecords(rows, *s.limit);

    // ---- projection ----
    ExecResult r;
    r.kind = ExecResult::Kind::ROWS;
    if (s.selectAll) {
        r.records = std::move(rows);
        r.columns = meta.columns;
    } else {
        std::vector<int32_t> cols;
        cols.reserve(s.columns.size());
        for (const std::string& name : s.columns) {
            int32_t col = findColumn(meta, name);
            if (col < 0) {
                return ExecResult::Error("Unknown column '" + name + "' in the select list");
            }
            cols.push_back(col);
            r.columns.push_back(meta.columns[col]);
        }
        r.records = project(rows, cols);
    }
    r.message = std::to_string(r.records.size()) + " row(s)";
    return r;
}
