#include "executor.h"
#include "../vector/vector_meta.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>

// ============================================================================
// Date -> epoch milliseconds
//
// Uses Howard Hinnant's days_from_civil algorithm rather than mktime/timegm:
// mktime() applies the host's local timezone (so the same query would mean
// different instants on different machines) and timegm() isn't portable to
// MSVC. This is pure arithmetic -- same answer everywhere.
// ============================================================================
static int64_t daysFromCivil(int y, unsigned m, unsigned d) {
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);              // [0, 399]
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;    // [0, 365]
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;             // [0, 146096]
    return era * 146097 + static_cast<int64_t>(doe) - 719468;
}

uint64_t Executor::dayStartMs(const DateLiteral& d) {
    int64_t days = daysFromCivil(d.year, static_cast<unsigned>(d.month), static_cast<unsigned>(d.day));
    int64_t ms = days * 86400LL * 1000LL;
    return ms < 0 ? 0 : static_cast<uint64_t>(ms);
}

uint64_t Executor::dayEndMs(const DateLiteral& d) {
    return dayStartMs(d) + 86400000ULL - 1ULL;
}

// ============================================================================
// Small helpers
// ============================================================================
static std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// Exact match first, then case-insensitive -- the lexer preserves identifier
// case, so `rollNo` and `rollno` should both find the same column rather than
// failing on a capitalisation difference.
int Executor::findColumn(const TableMeta& meta, const std::string& name) {
    for (int i = 0; i < meta.columnCount; ++i) {
        if (std::strcmp(meta.columns[i].name, name.c_str()) == 0) return i;
    }
    const std::string needle = lower(name);
    for (int i = 0; i < meta.columnCount; ++i) {
        if (lower(std::string(meta.columns[i].name)) == needle) return i;
    }
    return -1;
}

int Executor::primaryKeyColumn(const TableMeta& meta) {
    for (int i = 0; i < meta.columnCount; ++i) {
        if (meta.columns[i].isPK) return i;
    }
    return -1;
}

// Engine rows are all strings; `raw` already holds the literal exactly as the
// user typed it for numbers, and strVal holds the unescaped string body.
std::string Executor::valueToString(const Value& v) {
    switch (v.kind) {
        case Value::Kind::INT:    return v.raw;
        case Value::Kind::FLOAT:  return v.raw;
        case Value::Kind::STRING: return v.strVal;
    }
    return v.raw;
}

// Primary keys are stored under a canonical string: Table::getPrimaryKey()
// normalizes INT/FLOAT columns via stoi/stof + to_string before they ever hit
// the history index (so "07" and "7" land on the same key). Anything that
// looks a stored/scanned pk up directly -- like resolvePks()'s fast path --
// has to normalize a literal the same way first, or an equality lookup can
// silently miss a row whose key was written with different (but numerically
// identical) formatting than the query literal.
std::string Executor::canonicalPkValue(const Value& v, DataType pkType) {
    const std::string raw = valueToString(v);
    try {
        if (pkType == DataType::INT)   return std::to_string(std::stoi(raw));
        if (pkType == DataType::FLOAT) return std::to_string(std::stof(raw));
    } catch (const std::exception&) {
        // Not parseable as the column's numeric type; fall through and let
        // the value be used as-is so the caller's later validation/lookup
        // reports the real error instead of us masking it here.
    }
    return raw;
}

bool Executor::toOperator(CompareOp op, Operator& out) {
    switch (op) {
        case CompareOp::EQ: out = EQ; return true;
        case CompareOp::NE: out = NE; return true;
        case CompareOp::LT: out = LT; return true;
        case CompareOp::LE: out = LE; return true;
        case CompareOp::GT: out = GT; return true;
        case CompareOp::GE: out = GE; return true;
        case CompareOp::SIMILAR_TO: return false;   // handled separately
    }
    return false;
}

bool Executor::buildWhere(const TableMeta& meta, const Condition& cond,
                          WhereClause& out, ExecResult& err) {
    int col = findColumn(meta, cond.column);
    if (col < 0) {
        err = ExecResult::Error("Unknown column '" + cond.column + "' on table '" +
                                std::string(meta.name) + "'");
        return false;
    }
    Operator op;
    if (!toOperator(cond.op, op)) {
        err = ExecResult::Error("SIMILAR TO is only supported in SELECT ... WHERE "
                                "on a SEMANTIC column");
        return false;
    }
    out.column = col;
    out.op = op;
    out.value = valueToString(cond.value);
    return true;
}

Table* Executor::requireTable(const std::string& name, ExecResult& err) {
    Table* t = catalog.getTable(name);
    if (!t) err = ExecResult::Error("No such table: '" + name + "'");
    return t;
}

// Reduces a WHERE condition to the set of primary keys it matches. Fast path:
// `WHERE <pk> = <value>` needs no scan at all.
std::vector<std::string> Executor::resolvePks(Table& table, const Condition& cond, ExecResult& err) {
    const TableMeta& meta = table.getMeta();
    int pkCol = primaryKeyColumn(meta);
    if (pkCol < 0) {
        err = ExecResult::Error("Table '" + std::string(meta.name) + "' has no primary key");
        return {};
    }

    int col = findColumn(meta, cond.column);
    if (col < 0) {
        err = ExecResult::Error("Unknown column '" + cond.column + "' on table '" +
                                std::string(meta.name) + "'");
        return {};
    }

    if (col == pkCol && cond.op == CompareOp::EQ) {
        return { canonicalPkValue(cond.value, meta.columns[pkCol].type) };
    }

    WhereClause clause;
    if (!buildWhere(meta, cond, clause, err)) return {};

    std::vector<Record> matches = where(table.scanLatest(), meta, clause);
    std::vector<std::string> pks;
    pks.reserve(matches.size());
    for (const Record& r : matches) {
        if (pkCol < static_cast<int>(r.row.values.size())) pks.push_back(r.row.values[pkCol]);
    }
    return pks;
}

// ============================================================================
// Vector handles
//
// Opened lazily: only tables that actually declared a SEMANTIC column have a
// .vec file, and building the index is expensive enough not to do per-statement.
//
// The index is rebuilt when a handle is reopened so it also supports vector
// files created by an earlier MemoraDB session.
// ============================================================================
Executor::VectorHandles* Executor::vectorsFor(Table& table) {
    const TableMeta& meta = table.getMeta();
    std::string name(meta.name);

    auto it = vectors.find(name);
    if (it != vectors.end()) {
        if (!it->second.vt) return nullptr;
        vecMeta reader;
        it->second.vt->getMeta() = reader.readMetadata(name + ".vec");
        it->second.idx->buildIndex(*it->second.vt);
        return &it->second;
    }

    bool hasSemantic = false;
    for (int i = 0; i < meta.columnCount; ++i) {
        if (meta.columns[i].isSemantic) { hasSemantic = true; break; }
    }

    VectorHandles handles;
    std::filesystem::path vecPath = "data/" + name + "/" + name + ".vec";
    if (hasSemantic && std::filesystem::exists(vecPath)) {
        vecMeta reader;
        VectorMeta vm = reader.readMetadata(name + ".vec");
        handles.vt = std::make_unique<vecTable>(vm);
        handles.idx = std::make_unique<VectorIndex>();
        handles.idx->buildIndex(*handles.vt);
    }

    auto inserted = vectors.emplace(name, std::move(handles)).first;
    return inserted->second.vt ? &inserted->second : nullptr;
}

bool Executor::embedSemanticRow(const TableMeta& meta, const Row& row, float (&out)[VEC_DIM]) {
    std::string text;
    for (int i = 0; i < meta.columnCount; ++i) {
        if (meta.columns[i].isSemantic) text += std::string(meta.columns[i].name) + ": " + row.values[i] + " ";
    }
    return text.empty() || (embedder && embedder(text, out));
}

// ============================================================================
// Dispatch
// ============================================================================
ExecResult Executor::execute(const Statement& stmt) {
    return std::visit([this](auto&& s) -> ExecResult { return this->run(s); }, stmt);
}
