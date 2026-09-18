#pragma once
#include "../parser/ast.h"
#include "../catalog/catalog.h"
#include "../query/query.h"
#include "../vector/vecTable.h"
#include "../vector/semantic/vector_index.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct ExecResult {
    enum class Kind {
        OK,          
        ROWS,       
        DIFFS,       
        SEARCH,      
        ERROR        
    };
    Kind kind = Kind::OK;
    std::string message;
    std::vector<Record> records;
    std::vector<ColMeta> columns;  
    std::vector<Difference> diffs;
    std::vector<SearchResult> search;
    bool ok() const { return kind != Kind::ERROR; }
    static ExecResult Ok(std::string msg = "") {
        ExecResult r; r.kind = Kind::OK; r.message = std::move(msg); return r;
    }
    static ExecResult Error(std::string msg) {
        ExecResult r; r.kind = Kind::ERROR; r.message = std::move(msg); return r;
    }
};
using EmbeddingProvider = std::function<bool(const std::string& text, float (&out)[VEC_DIM])>;
class Executor {
public:
    explicit Executor(Catalog& catalog) : catalog(catalog) {}
    void setEmbeddingProvider(EmbeddingProvider provider) { embedder = std::move(provider); }
    ExecResult execute(const Statement& stmt);
    static uint64_t dayStartMs(const DateLiteral& d);
    static uint64_t dayEndMs(const DateLiteral& d);
    
private:
    Catalog& catalog;
    EmbeddingProvider embedder;
    struct VectorHandles {
        std::unique_ptr<vecTable> vt;
        std::unique_ptr<VectorIndex> idx;
    };
    std::unordered_map<std::string, VectorHandles> vectors;
    VectorHandles* vectorsFor(Table& table);
    bool embedSemanticRow(const TableMeta& meta, const Row& row, float (&out)[VEC_DIM]);

    ExecResult run(const CreateTableStmt& s);
    ExecResult run(const DropTableStmt& s);
    ExecResult run(const DescribeTableStmt& s);
    ExecResult run(const InsertStmt& s);
    ExecResult run(const UpdateStmt& s);
    ExecResult run(const DeleteStmt& s);
    ExecResult run(const SelectStmt& s);
    ExecResult run(const CompareStmt& s);
    ExecResult run(const EvolutionStmt& s);
    ExecResult run(const HistoryStmt& s);
    ExecResult run(const RollbackStmt& s);
    ExecResult run(const CompactStmt& s);

    Table* requireTable(const std::string& name, ExecResult& err);
    static int findColumn(const TableMeta& meta, const std::string& name);
    static int primaryKeyColumn(const TableMeta& meta);
    static std::string valueToString(const Value& v);
    static std::string canonicalPkValue(const Value& v, DataType pkType);
    static bool toOperator(CompareOp op, Operator& out);
    static bool buildWhere(const TableMeta& meta, const Condition& cond,
                           WhereClause& out, ExecResult& err);
    std::vector<std::string> resolvePks(Table& table, const Condition& cond, ExecResult& err);
};
