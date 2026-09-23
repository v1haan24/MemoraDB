#include "repl.h"
#include "help.h"
#include "terminal.h"
#include "../common/constants.h"
#include "../common/time_format.h"
#include "../lexer/lexer.h"
#include "../lexer/token.h"
#include "../parser/parser.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <stdlib.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

namespace {

volatile std::sig_atomic_t interrupted = 0;

void handleInterrupt(int) {
    interrupted = 1;
}

std::filesystem::path getModelDirectory() {
#ifdef _WIN32
    char* exePath = nullptr;
    if (_get_pgmptr(&exePath) != 0 || exePath == nullptr) {
        throw std::runtime_error("Unable to determine MemoraDB executable location.");
    }
    return std::filesystem::path(exePath).parent_path() / "models" / "all-MiniLM-L6-v2";
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size + 1);
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
        throw std::runtime_error("Unable to determine MemoraDB executable location.");
    }
    return std::filesystem::path(buffer.data()).parent_path() / "models" / "all-MiniLM-L6-v2";
#else
    std::vector<char> buffer(1024);
    const ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (length <= 0) {
        throw std::runtime_error("Unable to determine MemoraDB executable location.");
    }
    buffer[length] = '\0';
    return std::filesystem::path(buffer.data()).parent_path() / "models" / "all-MiniLM-L6-v2";
#endif
}

std::string trim(const std::string& s) {
    const auto a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return {};
    return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}

std::string lower(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

size_t findTerminator(const std::string& s) {
    bool single = false;
    bool dbl = false;
    bool escape = false;

    for (size_t i = 0; i < s.size(); ++i) {
        const char c = s[i];
        if (escape) {
            escape = false;
            continue;
        }
        if ((single || dbl) && c == '\\') {
            escape = true;
            continue;
        }
        if (c == '\'' && !dbl) {
            single = !single;
        } else if (c == '"' && !single) {
            dbl = !dbl;
        } else if (c == ';' && !single && !dbl) {
            return i;
        }
    }
    return std::string::npos;
}

std::string timestamp(uint64_t t) {
    return formatTimestamp(t);
}

std::string floatString(float x) {
    std::ostringstream s;
    s << std::fixed << std::setprecision(4) << x;
    return s.str();
}

std::string millisString(double x) {
    std::ostringstream s;
    s << std::fixed << std::setprecision(3) << x;
    return s.str();
}

term::Color tokenColor(TokenType t) {
    switch (t) {
        case TokenType::IDENTIFIER:      return term::GREEN;
        case TokenType::INTEGER_LITERAL:
        case TokenType::FLOAT_LITERAL:
        case TokenType::STRING_LITERAL:  return term::YELLOW;
        case TokenType::UNKNOWN:         return term::RED;
        case TokenType::END_OF_FILE:     return term::DIM;
        default:                         return term::CYAN;
    }
}

} // namespace

Repl::Repl() : executor(catalog) {
    term::init();
    term::setTitle("MemoraDB");

    embedder = std::make_unique<MiniLmEmbedder>(getModelDirectory().string());

    executor.setEmbeddingProvider([this](const std::string& text, float (&out)[VEC_DIM]) {
        try {
            const auto e = embedder->encode(text);
            if (e.size() != VEC_DIM) return false;
            std::copy(e.begin(), e.end(), out);
            return true;
        } catch (const std::exception& error) {
            std::cerr << "ONNX embedding failed: " << error.what() << '\n';
            return false;
        }
    });
}

void Repl::showTokens(const std::string& sql) {
    const auto start = std::chrono::steady_clock::now();
    Lexer lexer(sql);
    const auto ts = lexer.tokenize();

    std::cout << std::left << std::setw(5) << '#'
              << std::setw(20) << "type"
              << std::setw(10) << "line:col"
              << "value\n";

    for (size_t i = 0; i < ts.size(); ++i) {
        const auto& t = ts[i];
        std::cout << std::left << std::setw(5) << i
                  << std::setw(20) << term::paint(tokenTypeToString(t.type), tokenColor(t.type))
                  << std::setw(10) << (std::to_string(t.line) + ':' + std::to_string(t.column))
                  << t.value << '\n';

        if (t.type == TokenType::END_OF_FILE) break;
    }

    const auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::cout << "Tokens: " << ts.size() << " (" << std::fixed << std::setprecision(3) << ms << " ms)\n";
}

void Repl::dispatchMeta(const std::string& command) {
    std::istringstream in(command);
    std::string cmd;
    std::string arg;
    in >> cmd;
    std::getline(in, arg);
    arg = trim(arg);

    const auto c = lower(cmd);

    if (c == ".help" || c == ".h") {
        if (arg.empty()) {
            cli::printHelp();
        } else if (!cli::showHelpTopic(arg)) {
            std::cout << term::paint("No help topic for: ", term::YELLOW) << arg << '\n'
                      << "Available topics: CREATE, DROP, DESCRIBE, SHOW, INSERT, UPDATE, DELETE, SELECT, WHERE, SIMILAR, AS OF, ORDER BY, LIMIT, COMPARE, EVOLUTION, HISTORY, ROLLBACK, COMPACT, DATE, TYPES, CLEAR.\n";
        }
    } else if (c == ".about") {
        cli::printAbout();
    } else if (c == ".tokens") {
        if (arg.empty()) {
            std::cout << "usage: .tokens <statement>\n";
        } else {
            showTokens(arg);
        }
    } else if (c == ".history") {
        if (history.empty()) {
            std::cout << "(empty)\n";
        }
        for (size_t i = 0; i < history.size(); ++i) {
            std::cout << i + 1 << "  " << history[i] << '\n';
        }
    } else if (c == ".clear" || c == ".cls") {
        term::clear();
    } else if (c == ".exit" || c == ".quit" || c == ".q" || c == "exit" || c == "quit") {
        running = false;
    } else {
        std::cout << term::paint("Unknown command: ", term::RED) << cmd << " (try .help)\n";
    }
}

bool Repl::readStatement(std::string& statement, std::string& meta) {
    statement.clear();
    meta.clear();

    std::string buffer = pending;
    pending.clear();

    while (running) {
        std::cout << term::paint(buffer.empty() ? "memora> " : "   ...> ",
                                 buffer.empty()
                                     ? std::initializer_list<term::Color>{term::BOLD, term::MAGENTA}
                                     : std::initializer_list<term::Color>{term::DIM})
                  << std::flush;

        std::string line;
        if (!std::getline(std::cin, line)) {
            if (interrupted) {
                interrupted = 0;
                buffer.clear();
                std::cin.clear();
                continue;
            }
            running = false;
            return false;
        }

        if (buffer.empty() && trim(line).empty()) {
            continue;
        }

        if (buffer.empty() && line[0] == '.') {
            meta = trim(line);
            return true;
        }

        if (!buffer.empty()) {
            buffer += '\n';
        }
        buffer += line;

        const auto pos = findTerminator(buffer);
        if (pos == std::string::npos) {
            continue;
        }

        statement = trim(buffer.substr(0, pos));
        pending = trim(buffer.substr(pos + 1));
        return !statement.empty();
    }

    return false;
}

void Repl::printResult(const ExecResult& r) {
    if (!r.ok()) {
        std::cout << term::paint("Error: ", {term::BOLD, term::RED}) << r.message << '\n';
        return;
    }

    if (r.kind == ExecResult::Kind::OK) {
        std::cout << term::paint("OK ", {term::BOLD, term::GREEN})
                  << term::paint(r.message.empty() ? "OK" : r.message, term::CYAN)
                  << '\n';
        return;
    }

    std::vector<std::string> h;
    std::vector<std::vector<std::string>> rows;

    if (r.kind == ExecResult::Kind::ROWS) {
        for (const auto& c : r.columns) {
            h.push_back(c.name);
        }
        h.insert(h.end(), {"_timestamp", "_deleted"});

        for (const auto& x : r.records) {
            auto row = x.row.values;
            row.push_back(timestamp(x.timestamp));
            row.push_back(x.deleted ? "true" : "false");
            rows.push_back(std::move(row));
        }

        if (rows.empty()) {
            std::cout << term::paint("No rows returned", term::YELLOW) << '\n';
            return;
        }
    } else if (r.kind == ExecResult::Kind::DIFFS) {
        h = {"timestamp", "column", "before", "after"};

        for (const auto& x : r.diffs) {
            rows.push_back({timestamp(x.timestamp), x.column, x.before, x.after});
        }

        if (rows.empty()) {
            std::cout << term::paint("No differences found", term::YELLOW) << '\n';
            return;
        }
    } else {
        h = {"pk", "timestamp", "score"};
        for (const auto& c : r.columns) {
            h.push_back(c.name);
        }

        for (const auto& x : r.search) {
            rows.push_back({x.pk, timestamp(x.timestamp), floatString(x.score)});
            for (const auto& value : x.semanticValues) {
                rows.back().push_back(value);
            }
        }

        if (rows.empty()) {
            std::cout << term::paint("No semantic matches found", term::YELLOW) << '\n';
            return;
        }
    }

    term::printTable(h, rows);

    if (!r.message.empty()) {
        std::cout << term::paint(r.message, term::LIGHT_YELLOW) << '\n';
    }
}

void Repl::executeProgram(const std::string& input) {
    try {
        Lexer lexer(input);
        Parser parser(lexer.tokenize());

        for (const auto& statement : parser.parseProgram()) {
            const auto start = std::chrono::steady_clock::now();
            ExecResult result = executor.execute(statement);
            const auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

            printResult(result);
            std::cout << term::paint("Executed in " + millisString(ms) + " ms", term::LIGHT_YELLOW) << '\n';

            if (!result.ok()) {
                break;
            }
        }
    } catch (const ParseError& e) {
        std::cout << term::paint("Parse Error: ", {term::BOLD, term::RED}) << e.what() << '\n'
                  << term::paint("line " + std::to_string(e.line) + ", column " + std::to_string(e.column), term::GRAY) << '\n';
    } catch (const std::exception& e) {
        std::cout << term::paint("Error: ", {term::BOLD, term::RED}) << e.what() << '\n';
    }
}

void Repl::run() {
    cli::printBanner(embedder != nullptr);

    std::signal(SIGINT, handleInterrupt);

    while (running) {
        if (interrupted) {
            interrupted = 0;
            pending.clear();
            std::cout << '\n' << term::paint("Query cancelled.", term::YELLOW) << '\n';
            continue;
        }

        std::string sql;
        std::string meta;

        if (!readStatement(sql, meta)) {
            break;
        }

        if (!meta.empty()) {
            dispatchMeta(meta);
        } else if (!sql.empty()) {
            history.push_back(sql);
            executeProgram(sql + ';');
        }
    }

    std::cout << term::paint("Goodbye.", term::DIM) << '\n';
}

int Repl::runCommandLine(int argc, char** argv) {
    if (argc == 1) {
        run();
        return 0;
    }

    std::ostringstream in;
    for (int i = 1; i < argc; ++i) {
        if (i > 1) in << ' ';
        in << argv[i];
    }

    const auto input = trim(in.str());
    if (input.empty()) {
        return 0;
    }

    if (input[0] == '.') {
        dispatchMeta(input);
        return 0;
    }

    executeProgram(input.back() == ';' ? input : input + ';');
    return 0;
}

int main(int argc, char** argv) {
    if (!std::filesystem::exists("data") && std::filesystem::exists("../data")) {
        std::error_code ec;
        std::filesystem::current_path("..", ec);
    }
    return Repl().runCommandLine(argc, argv);
}