#include "../src/catalog/catalog.h"
#include "../src/engine/executor.h"
#include "../src/lexer/lexer.h"
#include "../src/parser/parser.h"
#include "../src/storage/table.h"

#include <algorithm>
#include <cctype>
#include <csignal>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace {

volatile std::sig_atomic_t interrupted = 0;

void handleInterrupt(int) {
    interrupted = 1;
}

class Terminal {
public:
    Terminal() : enabled(detectColor()) {}

    std::string accent(const std::string& text) const { return color(text, "36;1"); }
    std::string muted(const std::string& text) const { return color(text, "2"); }
    std::string success(const std::string& text) const { return color(text, "32;1"); }
    std::string error(const std::string& text) const { return color(text, "31;1"); }
    std::string warning(const std::string& text) const { return color(text, "33;1"); }
    std::string header(const std::string& text) const { return color(text, "1"); }

private:
    bool enabled = false;

    static bool detectColor() {
        if (std::getenv("NO_COLOR")) return false;
        if (std::getenv("MEMORA_NO_COLOR")) return false;
        if (std::getenv("TERM") && std::string(std::getenv("TERM")) == "dumb") return false;
#ifdef _WIN32
        return _isatty(_fileno(stdout)) != 0;
#else
        return isatty(STDOUT_FILENO) != 0;
#endif
    }

    std::string color(const std::string& text, const char* code) const {
        if (!enabled) return text;
        return std::string("\033[") + code + "m" + text + "\033[0m";
    }
};

std::string trim(const std::string& input) {
    auto first = std::find_if_not(input.begin(), input.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    auto last = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char ch) {
        return std::isspace(ch);
    }).base();

    if (first >= last) return "";
    return std::string(first, last);
}

std::string lower(std::string input) {
    for (char& ch : input) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return input;
}

bool endsWithSemicolon(const std::string& text) {
    std::string cleaned = trim(text);
    return !cleaned.empty() && cleaned.back() == ';';
}

std::string timestampString(uint64_t timestamp) {
    return std::to_string(timestamp);
}

std::string formatFloat(float value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(4) << value;
    return out.str();
}

std::string repeatText(const std::string& text, size_t count) {
    std::string out;
    out.reserve(text.size() * count);
    for (size_t i = 0; i < count; ++i) out += text;
    return out;
}

void printBanner(const Terminal& term) {
    std::cout << '\n';
    std::cout << term.accent("+----------------------------------------+") << '\n';
    std::cout << term.accent("|                MemoraDB                |") << '\n';
    std::cout << term.accent("|        Temporal + Semantic DB          |") << '\n';
    std::cout << term.accent("+----------------------------------------+") << '\n';
    std::cout << term.muted("Type .help for commands. End queries with ;") << "\n\n";
}

std::vector<std::string> recordRow(const Record& record, size_t width) {
    std::vector<std::string> row;
    row.reserve(width);
    for (size_t i = 0; i < width; ++i) {
        row.push_back(i < record.row.values.size() ? record.row.values[i] : "");
    }
    return row;
}

void printTable(const Terminal& term,
                const std::vector<std::string>& headers,
                const std::vector<std::vector<std::string>>& rows) {
    if (headers.empty()) return;

    std::vector<size_t> widths(headers.size());
    for (size_t i = 0; i < headers.size(); ++i) widths[i] = headers[i].size();

    for (const auto& row : rows) {
        for (size_t i = 0; i < headers.size(); ++i) {
            if (i < row.size()) widths[i] = std::max(widths[i], row[i].size());
        }
    }

    auto border = [&](const char* left, const char* mid, const char* right) {
        std::cout << term.muted(left);
        for (size_t i = 0; i < widths.size(); ++i) {
            std::cout << term.muted(repeatText("-", widths[i] + 2));
            std::cout << term.muted(i + 1 == widths.size() ? right : mid);
        }
        std::cout << '\n';
    };

    auto printRow = [&](const std::vector<std::string>& row) {
        std::cout << term.muted("|");
        for (size_t i = 0; i < widths.size(); ++i) {
            std::string cell = i < row.size() ? row[i] : "";
            std::cout << ' ' << std::left << std::setw(static_cast<int>(widths[i]))
                      << cell << ' ';
            std::cout << term.muted("|");
        }
        std::cout << '\n';
    };

    border("+", "+", "+");
    printRow(headers);
    border("+", "+", "+");
    for (const auto& row : rows) printRow(row);
    border("+", "+", "+");
}

void printRows(const Terminal& term, const ExecResult& result) {
    if (result.records.empty()) {
        std::cout << term.warning("No rows returned") << '\n';
        return;
    }

    std::vector<std::string> headers;
    headers.reserve(result.columns.size() + 2);
    for (const ColMeta& col : result.columns) headers.emplace_back(col.name);
    headers.emplace_back("_timestamp");
    headers.emplace_back("_deleted");

    std::vector<std::vector<std::string>> rows;
    rows.reserve(result.records.size());
    for (const Record& record : result.records) {
        std::vector<std::string> row = recordRow(record, result.columns.size());
        row.push_back(timestampString(record.timestamp));
        row.push_back(record.deleted ? "true" : "false");
        rows.push_back(std::move(row));
    }

    printTable(term, headers, rows);
}

void printDiffs(const Terminal& term, const ExecResult& result) {
    if (result.diffs.empty()) {
        std::cout << term.warning("No differences found") << '\n';
        return;
    }

    std::vector<std::vector<std::string>> rows;
    rows.reserve(result.diffs.size());
    for (const Difference& diff : result.diffs) {
        rows.push_back({
            timestampString(diff.timestamp),
            diff.column,
            diff.before,
            diff.after
        });
    }

    printTable(term, {"timestamp", "column", "before", "after"}, rows);
}

void printSearch(const Terminal& term, const ExecResult& result) {
    if (result.search.empty()) {
        std::cout << term.warning("No semantic matches found") << '\n';
        return;
    }

    std::vector<std::vector<std::string>> rows;
    rows.reserve(result.search.size());
    for (const SearchResult& hit : result.search) {
        rows.push_back({hit.pk, timestampString(hit.timestamp), formatFloat(hit.score)});
    }

    printTable(term, {"pk", "timestamp", "score"}, rows);
}

void printResult(const Terminal& term, const ExecResult& result) {
    if (!result.ok()) {
        std::cout << term.error("Error") << '\n';
        if (!result.message.empty()) std::cout << result.message << '\n';
        return;
    }

    switch (result.kind) {
        case ExecResult::Kind::OK:
            std::cout << term.success("OK ") << (result.message.empty() ? "OK" : result.message) << '\n';
            break;
        case ExecResult::Kind::ROWS:
            printRows(term, result);
            if (!result.message.empty()) std::cout << term.muted(result.message) << '\n';
            break;
        case ExecResult::Kind::DIFFS:
            printDiffs(term, result);
            if (!result.message.empty()) std::cout << term.muted(result.message) << '\n';
            break;
        case ExecResult::Kind::SEARCH:
            printSearch(term, result);
            if (!result.message.empty()) std::cout << term.muted(result.message) << '\n';
            break;
        case ExecResult::Kind::ERROR:
            break;
    }
}

void printHelp(const Terminal& term) {
    std::cout << term.header("MemoraDB CLI") << '\n';
    std::cout << "  .help           Show this help\n";
    std::cout << "  .clear          Clear the terminal\n";
    std::cout << "  .exit, .quit    Exit MemoraDB\n\n";

    std::cout << term.header("Supported query families") << '\n';
    std::cout << "  CREATE TABLE, DROP TABLE, DESCRIBE TABLE\n";
    std::cout << "  INSERT INTO, UPDATE, DELETE FROM\n";
    std::cout << "  SELECT ... FROM ... [AS OF | BETWEEN | SNAPSHOT] [WHERE] [ORDER BY] [LIMIT]\n";
    std::cout << "  HISTORY, COMPARE, EVOLUTION\n";
    std::cout << "  ROLLBACK, COMPACT TABLE\n\n";

    std::cout << term.header("Examples") << '\n';
    std::cout << "  CREATE TABLE Notes (id INT PRIMARY KEY, body STRING(200) SEMANTIC);\n";
    std::cout << "  INSERT INTO Notes VALUES (1, \"Temporal databases remember change\");\n";
    std::cout << "  SELECT * FROM Notes WHERE id = 1;\n";
    std::cout << "  HISTORY Notes WHERE id = 1;\n";
}

void clearScreen() {
    std::cout << "\033[2J\033[H";
    std::cout.flush();
}

bool handleMetaCommand(const Terminal& term, const std::string& input, bool& shouldExit) {
    if (input.empty() || input[0] != '.') return false;

    std::string command = lower(trim(input));
    if (command == ".help") {
        printHelp(term);
    } else if (command == ".exit" || command == ".quit") {
        shouldExit = true;
    } else if (command == ".clear") {
        clearScreen();
    } else {
        std::cout << term.error("Unknown command: ") << input << '\n';
        std::cout << term.muted("Type .help for available commands.") << '\n';
    }
    return true;
}

std::vector<Statement> parseInput(const std::string& input) {
    Lexer lexer(input);
    Parser parser(lexer.tokenize());
    return parser.parseProgram();
}

void executeInput(const Terminal& term, Executor& executor, const std::string& input) {
    try {
        std::vector<Statement> statements = parseInput(input);
        for (const Statement& statement : statements) {
            printResult(term, executor.execute(statement));
        }
    } catch (const ParseError& err) {
        std::cout << term.error("Parse Error") << '\n';
        std::cout << err.what() << '\n';
        std::cout << term.muted("line " + std::to_string(err.line) +
                                ", column " + std::to_string(err.column)) << '\n';
    } catch (const std::exception& err) {
        std::cout << term.error("Error") << '\n';
        std::cout << err.what() << '\n';
    }
}

} // namespace

int main(int argc, char** argv) {
    std::signal(SIGINT, handleInterrupt);

    Terminal term;
    Catalog catalog;
    Executor executor(catalog);
    executor.setEmbeddingProvider([](const std::string& text, float (&out)[VEC_DIM]) {
        std::vector<float> embedding = strToEmbed(text);
        if (embedding.size() != VEC_DIM) return false;

        std::copy(embedding.begin(), embedding.end(), out);
        return true;
    });

    if (argc > 1) {
        std::ostringstream query;
        for (int i = 1; i < argc; ++i) {
            if (i > 1) query << ' ';
            query << argv[i];
        }

        std::string input = trim(query.str());
        bool shouldExit = false;
        if (handleMetaCommand(term, input, shouldExit)) return 0;
        if (!input.empty() && !endsWithSemicolon(input)) input += ';';
        executeInput(term, executor, input);
        return 0;
    }

    printBanner(term);

    std::string buffer;
    bool shouldExit = false;

    while (!shouldExit) {
        if (interrupted) {
            interrupted = 0;
            buffer.clear();
            std::cout << '\n';
        }

        std::cout << term.accent(buffer.empty() ? "memora> " : "   ...> ");
        std::cout.flush();

        std::string line;
        if (!std::getline(std::cin, line)) {
            if (interrupted) {
                interrupted = 0;
                buffer.clear();
                std::cin.clear();
                std::cout << '\n';
                continue;
            }
            std::cout << '\n';
            break;
        }

        std::string cleaned = trim(line);
        if (buffer.empty() && cleaned.empty()) continue;

        if (buffer.empty() && handleMetaCommand(term, cleaned, shouldExit)) continue;

        if (!buffer.empty()) buffer += '\n';
        buffer += line;

        if (!endsWithSemicolon(buffer)) continue;

        executeInput(term, executor, buffer);
        buffer.clear();
        std::cout << '\n';
    }

    std::cout << term.muted("Goodbye.") << '\n';
    return 0;
}
