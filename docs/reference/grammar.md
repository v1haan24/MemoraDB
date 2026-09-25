# Formal Grammar & Syntax Notation

This page defines the notation conventions used throughout the MemoraDB documentation and provides the complete formal Extended Backus-Naur Form (EBNF) grammar for the MemoraDB SQL dialect.

---

## Syntax Notation Guide

The command reference pages in this documentation use standard typographical conventions to describe SQL syntax:

| Symbol | Meaning | Example | Explanation |
|---|---|---|---|
| `<...>` | **Placeholder** | `<table>`, `<column>` | Replace this token with your own table name, column name, or literal value. |
| `[ ... ]` | **Optional element** | `[WHERE <condition>]` | The enclosed clause or keyword may be included or omitted. |
| `\|` | **Alternative / Choice** | `ASC \| DESC` | Choose exactly one of the listed options. |
| `*` | **Zero or more** | `ColumnDef ( "," ColumnDef )*` | The preceding element may repeat zero or more times separated by commas. |
| `+` | **One or more** | `Digit+` | The preceding element must appear at least once. |
| `UPPERCASE` | **SQL Keyword** | `SELECT`, `FROM`, `WHERE` | Keywords must be written as shown (case-insensitive in MemoraDB). |

---

## EBNF Specification

```ebnf
(* Top-level Program *)
Program               ::= Statement*

Statement             ::= ( CreateTableStmt
                          | DropTableStmt
                          | DescribeTableStmt
                          | ShowTablesStmt
                          | InsertStmt
                          | UpdateStmt
                          | DeleteStmt
                          | SelectStmt
                          | CompareStmt
                          | EvolutionStmt
                          | HistoryStmt
                          | RollbackStmt
                          | CompactStmt ) ";"

(* DDL Statements *)
CreateTableStmt       ::= "CREATE" "TABLE" Identifier "(" ColumnDef ( "," ColumnDef )* ")"
ColumnDef             ::= Identifier DataType ( Modifier )*
Modifier              ::= "PRIMARY" "KEY" | "SEMANTIC"
DataType              ::= "INT"
                        | "FLOAT"
                        | "BOOL"
                        | "STRING" "(" IntegerLiteral ")"

DropTableStmt         ::= "DROP" "TABLE" Identifier
DescribeTableStmt     ::= "DESCRIBE" "TABLE" Identifier
ShowTablesStmt        ::= "SHOW" "TABLES"

(* DML Statements *)
InsertStmt            ::= "INSERT" "INTO" Identifier "VALUES" "(" Value ( "," Value )* ")"
UpdateStmt            ::= "UPDATE" Identifier "SET" Assignment ( "," Assignment )* ( "WHERE" Condition )?
Assignment            ::= Identifier "=" Value
DeleteStmt            ::= "DELETE" "FROM" Identifier ( "WHERE" Condition )?

(* Query Statements *)
SelectStmt            ::= "SELECT" SelectList "FROM" Identifier
                          ( TemporalClause | WhereClause )*
                          ( "ORDER" "BY" Identifier ( "ASC" | "DESC" )? )?
                          ( "LIMIT" IntegerLiteral )?

SelectList            ::= "*" | Identifier ( "," Identifier )*
TemporalClause        ::= "AS" "OF" DateLiteral
                        | "SNAPSHOT" DateLiteral
                        | "BETWEEN" DateLiteral "AND" DateLiteral
WhereClause           ::= "WHERE" Condition

(* Temporal Inspection & Maintenance *)
CompareStmt           ::= "COMPARE" Identifier "WHERE" Condition "BETWEEN" DateLiteral "AND" DateLiteral
EvolutionStmt         ::= "EVOLUTION" Identifier "WHERE" Condition "BETWEEN" DateLiteral "AND" DateLiteral
HistoryStmt           ::= "HISTORY" Identifier "WHERE" Condition
RollbackStmt          ::= "ROLLBACK" ( Identifier "WHERE" Condition | "TABLE" Identifier ) "TO" DateLiteral
CompactStmt           ::= "COMPACT" "TABLE" Identifier "TO" DateLiteral

(* Conditions & Operators *)
Condition             ::= Identifier ( CompareOp Value | "SIMILAR" "TO" StringLiteral )
CompareOp             ::= "=" | "!=" | "<" | "<=" | ">" | ">="

(* Literals & Values *)
Value                 ::= ( "-" )? ( IntegerLiteral | FloatLiteral ) | StringLiteral
DateLiteral           ::= DatePart ( TimePart )?
DatePart              ::= IntegerLiteral "-" IntegerLiteral "-" IntegerLiteral
TimePart              ::= IntegerLiteral ( ":" IntegerLiteral ( ":" ( IntegerLiteral | FloatLiteral ) )? )?

(* Lexer Terminals *)
Identifier            ::= [a-zA-Z_][a-zA-Z0-9_]*
IntegerLiteral        ::= [0-9]+
FloatLiteral          ::= [0-9]+ "." [0-9]+
StringLiteral         ::= '"' [^"\\]* '"' | "'" [^'\\]* "'"
```

---

## Grammar Rules & Parser Invariants

1. **Keyword Case-Insensitivity**: All keyword terminals (`SELECT`, `CREATE`, `TABLE`, etc.) are matched case-insensitively.
2. **Clause Ordering in SELECT**: The parser allows `<TemporalClause>` and `<WhereClause>` to appear in either order.
3. **Fixed Clause Ordering in Other Statements**: In `COMPARE` and `EVOLUTION`, `WHERE` must strictly precede `BETWEEN ... AND ...`. In `ROLLBACK`, `WHERE` must strictly precede `TO`.
4. **Single Condition Rule**: The `Condition` rule supports exactly one comparison or semantic clause. No `AND` / `OR` boolean expressions are permitted within a `WHERE` clause.
5. **Date Formats**: The `TimePart` rule supports hour alone (`HH`), hour and minute (`HH:MM`), and hour, minute, second with optional milliseconds (`HH:MM:SS[.mmm]`).
