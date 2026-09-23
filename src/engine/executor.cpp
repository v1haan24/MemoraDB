#include "executor.h"
#include "../vector/vector_meta.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <ctime>
#include <filesystem>

static int64_t daysFromCivil(int32_t y, uint32_t m, uint32_t d) {
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const uint32_t yoe = static_cast<uint32_t>(y - era * 400);              
    const uint32_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;    
    const uint32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;            
    return era * 146097 + static_cast<int64_t>(doe) - 719468;
}

static int64_t localCivilToEpochSeconds(const DateLiteral& d, int32_t dayOffset) {
    std::tm tm{};
    tm.tm_year = d.year - 1900;
    tm.tm_mon = d.month - 1;
    tm.tm_mday = d.day + dayOffset;
    tm.tm_hour = d.hasTime ? d.hour : 0;
    tm.tm_min = d.hasTime ? d.minute : 0;
    tm.tm_sec = d.hasTime ? d.second : 0;
    tm.tm_isdst = -1;

    const std::time_t t = std::mktime(&tm);
    if (t != static_cast<std::time_t>(-1)) {
        return static_cast<int64_t>(t);
    }  
    int64_t days = daysFromCivil(d.year, static_cast<uint32_t>(d.month),
                                 static_cast<uint32_t>(d.day)) + dayOffset;
    int64_t secs = days * 86400LL;
    if (d.hasTime) {
        secs += static_cast<int64_t>(d.hour) * 3600LL +
                static_cast<int64_t>(d.minute) * 60LL +
                static_cast<int64_t>(d.second);
    }
    return secs;
}

uint64_t Executor::dayStartMs(const DateLiteral& d) {
    int64_t ms = localCivilToEpochSeconds(d, 0) * 1000LL;
    if (d.hasTime) {
        ms += static_cast<int64_t>(d.millis);
    }
    return ms < 0 ? 0 : static_cast<uint64_t>(ms);
}

uint64_t Executor::dayEndMs(const DateLiteral& d) {
    if (d.hasTime) return dayStartMs(d);   
    int64_t ms = localCivilToEpochSeconds(d, 1) * 1000LL - 1LL;
    return ms < 0 ? 0 : static_cast<uint64_t>(ms);
}

static std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

int32_t Executor::findColumn(const TableMeta& meta, const std::string& name) {
    for (uint32_t i = 0; i < meta.columnCount; ++i) {
        if (std::strcmp(meta.columns[i].name, name.c_str()) == 0) return i;
    }
    const std::string needle = lower(name);
    for (uint32_t i = 0; i < meta.columnCount; ++i) {
        if (lower(std::string(meta.columns[i].name)) == needle) return i;
    }
    return -1;
}

int32_t Executor::primaryKeyColumn(const TableMeta& meta) {
    for (uint32_t i = 0; i < meta.columnCount; ++i) {
        if (meta.columns[i].isPK) return i;
    }
    return -1;
}

std::string Executor::valueToString(const Value& v) {
    switch (v.kind) {
        case Value::Kind::INT:    return v.raw;
        case Value::Kind::FLOAT:  return v.raw;
        case Value::Kind::STRING: return v.strVal;
    }
    return v.raw;
}

std::string Executor::canonicalPkValue(const Value& v, DataType pkType) {
    const std::string raw = valueToString(v);
    try {
        if (pkType == DataType::INT)   return std::to_string(std::stoi(raw));
        if (pkType == DataType::FLOAT) return std::to_string(std::stof(raw));
    } catch (const std::exception&) {
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
        case CompareOp::SIMILAR_TO: return false;   
    }
    return false;
}

bool Executor::buildWhere(const TableMeta& meta, const Condition& cond,
                          WhereClause& out, ExecResult& err) {
    int32_t col = findColumn(meta, cond.column);
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

std::vector<std::string> Executor::resolvePks(Table& table, const Condition& cond, ExecResult& err) {
    const TableMeta& meta = table.getMeta();
    int32_t pkCol = primaryKeyColumn(meta);
    if (pkCol < 0) {
        err = ExecResult::Error("Table '" + std::string(meta.name) + "' has no primary key");
        return {};
    }

    int32_t col = findColumn(meta, cond.column);
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
        if (pkCol < static_cast<int32_t>(r.row.values.size())) pks.push_back(r.row.values[pkCol]);
    }
    return pks;
}

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
    for (uint32_t i = 0; i < meta.columnCount; ++i) {
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
    for (uint32_t i = 0; i < meta.columnCount; ++i) {
        if (meta.columns[i].isSemantic) text += std::string(meta.columns[i].name) + ": " + row.values[i] + " ";
    }
    return text.empty() || (embedder && embedder(text, out));
}

ExecResult Executor::execute(const Statement& stmt) {
    return std::visit([this](auto&& s) -> ExecResult { return this->run(s); }, stmt);
}