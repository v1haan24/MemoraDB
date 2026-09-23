#include "help.h"
#include "terminal.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

const char* const kLogoMemora[8] = {
    R"ART(888b     d888 8888888888 888b     d888  .d88888b.  8888888b.         d8888)ART",
    R"ART(8888b   d8888 888        8888b   d8888 d88P" "Y88b 888   Y88b       d88888)ART",
    R"ART(88888b.d88888 888        88888b.d88888 888     888 888    888      d88P888)ART",
    R"ART(888Y88888P888 8888888    888Y88888P888 888     888 888   d88P     d88P 888)ART",
    R"ART(888 Y888P 888 888        888 Y888P 888 888     888 8888888P"     d88P  888)ART",
    R"ART(888  Y8P  888 888        888  Y8P  888 888     888 888 T88b     d88P   888)ART",
    R"ART(888   "   888 888        888   "   888 Y88b. .d88P 888  T88b   d8888888888)ART",
    R"ART(888       888 8888888888 888       888  "Y88888P"  888   T88b d88P     888)ART"
};

const char* const kLogoDb[8] = {
    R"ART(8888888b.  888888b.  )ART",
    R"ART(888  "Y88b 888  "88b )ART",
    R"ART(888    888 888  .88P )ART",
    R"ART(888    888 8888888K. )ART",
    R"ART(888    888 888  "Y88b)ART",
    R"ART(888    888 888    888)ART",
    R"ART(888  .d88P 888   d88P)ART",
    R"ART(8888888P"  8888888P" )ART"
};

const int kLogoMemoraWidth = 74;
const int kLogoDbWidth = 21;

std::string trim(const std::string& s) {
    const auto a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return {};
    return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}

std::string upper(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return s;
}

std::string collapseSpaces(const std::string& s) {
    std::istringstream in(trim(s));
    std::string out;
    std::string word;
    while (in >> word) {
        if (!out.empty()) out += ' ';
        out += word;
    }
    return out;
}

void printTopic(const std::string& title, const std::vector<std::string>& lines) {
    std::cout << term::paint(title, {term::BOLD, term::CYAN}) << '\n';
    for (const std::string& line : lines) {
        std::cout << line << '\n';
    }
}

} // namespace

void cli::printBanner(bool semanticReady) {
    const std::string dot = "\342\227\217";
    const std::string bar = "\342\224\200";
    const int cols = term::width();
    const int indent = 2;
    const std::string pad(indent, ' ');

    std::cout << '\n';

    if (cols >= indent + kLogoMemoraWidth + 2 + kLogoDbWidth + 2) {
        const int total = kLogoMemoraWidth + 2 + kLogoDbWidth;
        for (int r = 0; r < 8; ++r) {
            std::cout << pad << term::gradientLine(std::string(kLogoMemora[r]) + "  " + kLogoDb[r], r, 8, total) << '\n';
        }
    } else if (cols >= indent + kLogoMemoraWidth + 2) {
        for (int r = 0; r < 8; ++r) {
            std::cout << pad << term::gradientLine(kLogoMemora[r], r, 16, kLogoMemoraWidth) << '\n';
        }
        const std::string dbPad(static_cast<size_t>((kLogoMemoraWidth - kLogoDbWidth) / 2), ' ');
        for (int r = 0; r < 8; ++r) {
            std::cout << pad << dbPad << term::gradientLine(kLogoDb[r], r + 8, 16, kLogoMemoraWidth) << '\n';
        }
    } else {
        std::cout << pad << term::gradientText("M E M O R A   D B") << '\n';
    }

    std::cout << '\n';

    int ruleWidth = cols - indent * 2;
    if (ruleWidth > kLogoMemoraWidth) ruleWidth = kLogoMemoraWidth;
    if (ruleWidth < 20) ruleWidth = 20;

    std::cout << pad << term::rgbPaint("semantic + temporal dbms", term::Rgb{255, 214, 102}, true) << '\n'
              << pad << term::gradientRule(bar, ruleWidth) << "\n\n";

    auto led = [&](const std::string& label, bool ok) {
        std::string cell = ok ? term::paint(dot, {term::BOLD, term::GREEN})
                              : term::paint(dot, {term::BOLD, term::RED});
        cell += "  ";
        cell += ok ? label : term::paint(label + " (off)", term::GRAY);
        return cell;
    };

    auto cellPad = [](const std::string& label, bool ok) {
        const size_t visible = 3 + label.size() + (ok ? 0 : 6);
        const size_t target = 36;
        return std::string(visible < target ? target - visible : 2, ' ');
    };

    const std::string l1 = "temporal engine ready";
    const std::string l2 = "append-only storage ready";
    const std::string l3 = "semantic vector search ready";
    const std::string l4 = "embedding model ready";

    if (cols >= 76) {
        std::cout << pad << led(l1, true) << cellPad(l1, true) << led(l2, true) << '\n'
                  << pad << led(l3, semanticReady) << cellPad(l3, semanticReady) << led(l4, semanticReady) << "\n\n";
    } else {
        std::cout << pad << led(l1, true) << '\n'
                  << pad << led(l2, true) << '\n'
                  << pad << led(l3, semanticReady) << '\n'
                  << pad << led(l4, semanticReady) << "\n\n";
    }

    std::cout << pad << term::paint("MemoraDB is a C++ append-only temporal database built from scratch featuring", term::GRAY) << '\n'
              << pad << term::paint("a custom binary storage engine, schema serialization, immutable versioned records,", term::GRAY) << '\n'
              << pad << term::paint("crash recovery, temporal indexing, snapshots, rollback, time-travel queries,", term::GRAY) << '\n'
              << pad << term::paint("semantic vector search, and a SQL-like query engine.", term::GRAY) << '\n'
              << pad << term::paint("GitHub Link: ", term::GREEN) << '\n'
              << pad << term::paint("https://github.com/v1haan24/MemoraDB.git", term::WHITE) << "\n\n"
              << pad << term::paint("Type ", term::GRAY)
              << term::rgbPaint(".help", term::Rgb{0, 229, 255}, true)
              << term::paint(" for commands, ", term::GRAY)
              << term::rgbPaint(".about", term::Rgb{192, 72, 255}, true)
              << term::paint(" for info, ", term::GRAY)
              << term::rgbPaint(".exit", term::Rgb{255, 84, 160}, true)
              << term::paint(" to quit.", term::GRAY)
              << "\n\n";
}

void cli::printHelp() {
    std::cout << term::paint("Commands", {term::BOLD, term::CYAN}) << '\n'
              << "  .help [cmd]        show help, or syntax for a command\n"
              << "  .tokens <stmt>     show lexer tokens\n"
              << "  .history           show command history\n"
              << "  .clear             clear screen\n"
              << "  .about             about MemoraDB\n"
              << "  .exit/.quit/.q     exit shell\n\n"
              << "SQL may span lines; terminate statements with ';'.\n"
              << "Try .help SELECT, .help CREATE TABLE, .help WHERE, or .help DATE.\n";
}

void cli::printAbout() {
    std::cout << term::paint("MemoraDB -", {term::BOLD, term::MAGENTA}) << '\n'
              << term::paint("A modular C++ append-only temporal database built from scratch featuring", term::GRAY) << '\n'
              << term::paint("a custom binary storage engine, schema serialization, immutable versioned records,", term::GRAY) << '\n'
              << term::paint("crash recovery, temporal indexing, snapshots, rollback, time-travel queries,", term::GRAY) << '\n'
              << term::paint("semantic vector search, and a SQL-like query engine.", term::GRAY) << '\n'
              << term::paint("GitHub Link: ", term::GREEN) << '\n'
              << term::paint("https://github.com/v1haan24/MemoraDB.git\n", term::WHITE);
}

bool cli::showHelpTopic(const std::string& arg) {
    std::string topic = upper(collapseSpaces(arg));
    if (!topic.empty() && topic.front() == '.') {
        topic.erase(topic.begin());
    }

    if (topic == "HELP") {
        printTopic(".help", {
            "Syntax:",
            "  .help",
            "  .help <cmd>",
            "",
            "Examples:",
            "  .help SELECT",
            "  .help CREATE TABLE",
            "  .help WHERE"
        });
        return true;
    }

    if (topic == "CLEAR" || topic == "CLS") {
        printTopic(".clear", {
            "Syntax:",
            "  .clear",
            "  .cls",
            "",
            "Clears the REPL screen. No semicolon is needed."
        });
        return true;
    }

    if (topic == "TOKENS") {
        printTopic(".tokens", {
            "Syntax:",
            "  .tokens <statement>",
            "",
            "Example:",
            "  .tokens SELECT * FROM notes;"
        });
        return true;
    }

    if (topic == "HISTORY") {
        printTopic(".history / HISTORY", {
            "Meta-command syntax:",
            "  .history",
            "",
            "SQL syntax:",
            "  HISTORY <table> WHERE <condition>;",
            "",
            "Example:",
            "  HISTORY notes WHERE id = 1;"
        });
        return true;
    }

    if (topic == "EXIT" || topic == "QUIT") {
        printTopic("exit", {
            "Syntax:",
            "  .exit",
            "  .quit",
            "  .q",
            "  exit",
            "  quit"
        });
        return true;
    }

    if (topic == "CREATE" || topic == "CREATE TABLE") {
        printTopic("CREATE TABLE", {
            "Syntax:",
            "  CREATE TABLE <table> (",
            "      <col> INT|FLOAT|BOOL [PRIMARY KEY],",
            "      <col> STRING(<size>) [PRIMARY KEY] [SEMANTIC]",
            "  );",
            "",
            "Notes:",
            "  Exactly one PRIMARY KEY is required.",
            "  SEMANTIC is supported only on STRING columns.",
            "",
            "Example:",
            "  CREATE TABLE notes (id INT PRIMARY KEY, body STRING(500) SEMANTIC);"
        });
        return true;
    }

    if (topic == "DROP" || topic == "DROP TABLE") {
        printTopic("DROP TABLE", {
            "Syntax:",
            "  DROP TABLE <table>;",
            "",
            "Example:",
            "  DROP TABLE notes;"
        });
        return true;
    }

    if (topic == "DESCRIBE" || topic == "DESCRIBE TABLE") {
        printTopic("DESCRIBE TABLE", {
            "Syntax:",
            "  DESCRIBE TABLE <table>;",
            "",
            "Example:",
            "  DESCRIBE TABLE notes;"
        });
        return true;
    }

    if (topic == "SHOW" || topic == "SHOW TABLES") {
        printTopic("SHOW TABLES", {
            "Syntax:",
            "  SHOW TABLES;"
        });
        return true;
    }

    if (topic == "INSERT" || topic == "INSERT INTO") {
        printTopic("INSERT", {
            "Syntax:",
            "  INSERT INTO <table> VALUES (<v1>, <v2>, ...);",
            "",
            "Notes:",
            "  Values are positional and must match the table column order.",
            "  INSERT always creates a new version of the row.",
            "",
            "Example:",
            "  INSERT INTO notes VALUES (1, 'release notes');"
        });
        return true;
    }

    if (topic == "UPDATE") {
        printTopic("UPDATE", {
            "Syntax:",
            "  UPDATE <table> SET <col1> = <value1>[, <col2> = <value2> ...] [WHERE <condition>];",
            "",
            "Notes:",
            "  Without WHERE, every current row is updated.",
            "  The primary-key column cannot be updated.",
            "",
            "Example:",
            "  UPDATE notes SET body = 'fixed text' WHERE id = 1;"
        });
        return true;
    }

    if (topic == "DELETE" || topic == "DELETE FROM") {
        printTopic("DELETE", {
            "Syntax:",
            "  DELETE FROM <table> [WHERE <condition>];",
            "",
            "Notes:",
            "  Without WHERE, every current row is deleted.",
            "",
            "Example:",
            "  DELETE FROM notes WHERE id = 1;"
        });
        return true;
    }

    if (topic == "SELECT") {
        printTopic("SELECT", {
            "Syntax:",
            "  SELECT * | <col1>[, <col2> ...]",
            "  FROM <table>",
            "  [AS OF <date> | SNAPSHOT <date> | BETWEEN <date1> AND <date2>]",
            "  [WHERE <condition>]",
            "  [ORDER BY <col> [ASC|DESC]]",
            "  [LIMIT <n>];",
            "",
            "Notes:",
            "  WHERE and the temporal clause can appear in either order.",
            "  SIMILAR TO is supported only on SEMANTIC columns.",
            "",
            "Examples:",
            "  SELECT * FROM notes;",
            "  SELECT id, body FROM notes AS OF 2026-09-20 WHERE id = 1;",
            "  SELECT * FROM notes WHERE body SIMILAR TO 'database search' LIMIT 5;"
        });
        return true;
    }

    if (topic == "WHERE") {
        printTopic("WHERE", {
            "Syntax:",
            "  WHERE <col> = <value>",
            "  WHERE <col> != <value>",
            "  WHERE <col> < <value>",
            "  WHERE <col> <= <value>",
            "  WHERE <col> > <value>",
            "  WHERE <col> >= <value>",
            "  WHERE <semantic_col> SIMILAR TO '<query text>'",
            "",
            "Notes:",
            "  One condition is supported. AND/OR chains are not supported.",
            "  SIMILAR TO works in SELECT only."
        });
        return true;
    }

    if (topic == "SIMILAR" || topic == "SIMILAR TO" || topic == "SEMANTIC") {
        printTopic("SEMANTIC SEARCH", {
            "Syntax:",
            "  SELECT * FROM <table> WHERE <semantic_col> SIMILAR TO '<query text>' [LIMIT <n>];",
            "",
            "Notes:",
            "  <semantic_col> must be declared SEMANTIC in CREATE TABLE.",
            "  LIMIT defaults to 10 when omitted.",
            "  AS OF, SNAPSHOT, and BETWEEN can be combined with SIMILAR TO.",
            "",
            "Example:",
            "  SELECT * FROM notes WHERE body SIMILAR TO 'rollback bug' AS OF 2026-09-20 LIMIT 5;"
        });
        return true;
    }

    if (topic == "AS OF" || topic == "SNAPSHOT" || topic == "BETWEEN" || topic == "TEMPORAL") {
        printTopic("TEMPORAL SELECT", {
            "Syntax:",
            "  SELECT * FROM <table> AS OF <date> [WHERE <condition>];",
            "  SELECT * FROM <table> SNAPSHOT <date> [WHERE <condition>];",
            "  SELECT * FROM <table> BETWEEN <date1> AND <date2> [WHERE <condition>];",
            "",
            "Notes:",
            "  AS OF and SNAPSHOT return a snapshot at that date or instant.",
            "  BETWEEN returns versions in the date range.",
            "  WHERE can appear before or after the temporal clause."
        });
        return true;
    }

    if (topic == "COMPARE") {
        printTopic("COMPARE", {
            "Syntax:",
            "  COMPARE <table> WHERE <condition> BETWEEN <date1> AND <date2>;",
            "",
            "Example:",
            "  COMPARE notes WHERE id = 1 BETWEEN 2026-09-01 AND 2026-09-20;"
        });
        return true;
    }

    if (topic == "EVOLUTION") {
        printTopic("EVOLUTION", {
            "Syntax:",
            "  EVOLUTION <table> WHERE <condition> BETWEEN <date1> AND <date2>;",
            "",
            "Example:",
            "  EVOLUTION notes WHERE id = 1 BETWEEN 2026-09-01 AND 2026-09-20;"
        });
        return true;
    }

    if (topic == "ROLLBACK") {
        printTopic("ROLLBACK", {
            "Syntax:",
            "  ROLLBACK <table> WHERE <condition> TO <date>;",
            "  ROLLBACK TABLE <table> TO <date>;",
            "",
            "Examples:",
            "  ROLLBACK notes WHERE id = 1 TO 2026-09-20;",
            "  ROLLBACK TABLE notes TO 2026-09-20;"
        });
        return true;
    }

    if (topic == "COMPACT" || topic == "COMPACT TABLE") {
        printTopic("COMPACT TABLE", {
            "Syntax:",
            "  COMPACT TABLE <table> TO <date>;",
            "",
            "Example:",
            "  COMPACT TABLE notes TO 2026-09-20;"
        });
        return true;
    }

    if (topic == "DATE" || topic == "TIME" || topic == "TIMESTAMP") {
        printTopic("DATE / TIMESTAMP", {
            "Supported forms:",
            "  YYYY-MM-DD",
            "  YYYY-MM-DD HH",
            "  YYYY-MM-DD HH:MM",
            "  YYYY-MM-DD HH:MM:SS",
            "  YYYY-MM-DD HH:MM:SS.mmm",
            "",
            "Example:",
            "  2026-09-20 14:30:45.250"
        });
        return true;
    }

    if (topic == "ORDER" || topic == "ORDER BY") {
        printTopic("ORDER BY", {
            "Syntax:",
            "  SELECT ... ORDER BY <col> [ASC|DESC];",
            "",
            "Notes:",
            "  ORDER BY is supported in SELECT.",
            "  ASC is the default when ASC/DESC is omitted.",
            "",
            "Example:",
            "  SELECT * FROM notes ORDER BY id DESC LIMIT 5;"
        });
        return true;
    }

    if (topic == "LIMIT") {
        printTopic("LIMIT", {
            "Syntax:",
            "  SELECT ... LIMIT <n>;",
            "",
            "Notes:",
            "  LIMIT is supported in SELECT.",
            "  Semantic search defaults to 10 results when LIMIT is omitted.",
            "",
            "Example:",
            "  SELECT * FROM notes WHERE body SIMILAR TO 'search text' LIMIT 5;"
        });
        return true;
    }

    if (topic == "TYPE" || topic == "TYPES" || topic == "DATA TYPES") {
        printTopic("DATA TYPES", {
            "Supported types:",
            "  INT",
            "  FLOAT",
            "  BOOL",
            "  STRING(<size>)",
            "",
            "Column modifiers:",
            "  PRIMARY KEY",
            "  SEMANTIC"
        });
        return true;
    }

    return false;
}
