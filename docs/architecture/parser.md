# Parser Architecture

The parser transforms the flat token stream produced by the lexer into structured, strongly-typed Abstract Syntax Tree (AST) nodes using recursive-descent parsing.

---

## Source Location

* **Headers**: `src/parser/parser.h`, `src/parser/ast.h`
* **Implementations**:
  * `parser_core.cpp` — Token navigation, expectation checks, and mismatch reporting.
  * `parser_statement.cpp` — Top-level statement dispatch.
  * `parser_select.cpp` — `SELECT` statement and flexible clause ordering.
  * `parser_ddl.cpp` — `CREATE TABLE`, `DROP TABLE`, `DESCRIBE TABLE`, `SHOW TABLES`.
  * `parser_dml.cpp` — `INSERT`, `UPDATE`, `DELETE`.
  * `parser_compare.cpp` — `COMPARE`, `EVOLUTION`, `HISTORY`.
  * `parser_rollback.cpp` — `ROLLBACK` and `COMPACT`.
  * `parser_leaf.cpp` — Literals, values, date/timestamp parsing, operators, conditions.

---

## The Abstract Syntax Tree (AST)

MemoraDB uses modern C++17 type-safe unions (`std::variant`) to represent statements:

```cpp
using Statement = std::variant<
    CreateTableStmt,
    DropTableStmt,
    DescribeTableStmt,
    ShowTablesStmt,
    InsertStmt,
    UpdateStmt,
    DeleteStmt,
    SelectStmt,
    CompareStmt,
    EvolutionStmt,
    HistoryStmt,
    RollbackStmt,
    CompactStmt
>;
```

### Key AST Structures (`src/parser/ast.h`)

* `SelectStmt`: Contains `selectAll`, `columns`, `tableName`, `temporalMode`, optional date literals (`asOfDate`, `betweenStart`, `betweenEnd`, `snapshotDate`), optional `where` condition, `orderByColumn`, `orderDescending`, and `limit`.
* `Condition`: Contains `column`, `CompareOp` (`EQ`, `NE`, `LT`, `LE`, `GT`, `GE`, `SIMILAR_TO`), and `Value`.
* `DateLiteral`: Encapsulates year, month, day, hour, minute, second, millisecond, and `hasTime` flag.
* `Value`: Type-tagged union storing `raw` text, `intVal`, `floatVal`, and `strVal`.

---

## Recursive-Descent Mechanics

The parser maintains a cursor `size_t current` over `std::vector<Token>`:

* `peek(offset)`: Inspects upcoming tokens without consuming them.
* `match(type)`: Consumes the current token if it matches `type`; returns `false` otherwise.
* `expect(type, context)`: Asserts that the next token matches `type`. If it does not, throws a detailed `ParseError`.

### Flexible Clause Parsing in `SELECT`
The parser supports writing `<temporal-clause>` and `WHERE` in either order using a lookahead loop:

```cpp
for (int32_t guard = 0; guard < 2; ++guard) {
    if (stmt.temporalMode == TemporalMode::NONE &&
        (check(TokenType::AS) || check(TokenType::BETWEEN) || check(TokenType::SNAPSHOT))) {
        parseTemporalClause(stmt);
        continue;
    }
    if (!stmt.where && match(TokenType::WHERE)) {
        stmt.where = parseCondition();
        continue;
    }
    break;
}
```

---

## Calendar & Timestamp Validation

In `parser_leaf.cpp`, date literals undergo strict calendar validation:

```cpp
static bool isLeapYear(int32_t year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int32_t daysInMonth(int32_t year, int32_t month) {
    static const int32_t lengths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year)) return 29;
    return lengths[month - 1];
}
```

If a user provides `2026-02-29` (a non-leap year), the parser throws:
```text
Invalid day 29 in date literal (month 2 of year 2026 has 28 days)
```

---

## Error Handling

Errors throw a `ParseError` struct containing the error message, source line, and column:

```cpp
struct ParseError : std::runtime_error {
    int32_t line;
    int32_t column;
};
```

The REPL catches `ParseError` and prints a clear diagnostic pointing to the exact character where the failure occurred.
