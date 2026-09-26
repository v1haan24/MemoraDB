#include "repl.h"
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

    if (
        _get_pgmptr(
            &exePath
        ) != 0 ||
        exePath == nullptr
    ) {

        throw std::runtime_error(
            "Unable to determine MemoraDB executable location."
        );

    }

    return std::filesystem::path(
        exePath
    ).parent_path()
        / "models"
        / "all-MiniLM-L6-v2";

#elif defined(__APPLE__)

    uint32_t size = 0;

    _NSGetExecutablePath(
        nullptr,
        &size
    );

    std::vector<char> buffer(
        size + 1
    );

    if (
        _NSGetExecutablePath(
            buffer.data(),
            &size
        ) != 0
    ) {

        throw std::runtime_error(
            "Unable to determine MemoraDB executable location."
        );

    }

    return std::filesystem::path(
        buffer.data()
    ).parent_path()
        / "models"
        / "all-MiniLM-L6-v2";

#else

    std::vector<char> buffer(
        1024
    );

    const ssize_t length =
        readlink(
            "/proc/self/exe",
            buffer.data(),
            buffer.size() - 1
        );

    if (
        length <= 0
    ) {

        throw std::runtime_error(
            "Unable to determine MemoraDB executable location."
        );

    }

    buffer[length] = '\0';

    return std::filesystem::path(
        buffer.data()
    ).parent_path()
        / "models"
        / "all-MiniLM-L6-v2";

#endif

}


std::string trim(
    const std::string& s
) {

    const auto a =
        s.find_first_not_of(
            " \t\r\n"
        );


    if (
        a == std::string::npos
    ) {

        return {};

    }


    return s.substr(
        a,
        s.find_last_not_of(
            " \t\r\n"
        ) - a + 1
    );

}


std::string lower(
    std::string s
) {

    for (
        char& c : s
    ) {

        c =
            static_cast<char>(
                std::tolower(
                    static_cast<unsigned char>(c)
                )
            );

    }


    return s;

}


std::string upper(
    std::string s
) {

    for (
        char& c : s
    ) {

        c =
            static_cast<char>(
                std::toupper(
                    static_cast<unsigned char>(c)
                )
            );

    }


    return s;

}


std::string collapseSpaces(
    const std::string& s
) {

    std::istringstream in(
        trim(s)
    );


    std::string out;
    std::string word;


    while (
        in >> word
    ) {

        if (
            !out.empty()
        ) {

            out += ' ';

        }


        out += word;

    }


    return out;

}


size_t findTerminator(
    const std::string& s
) {

    bool single = false;
    bool dbl = false;
    bool escape = false;


    for (
        size_t i = 0;
        i < s.size();
        ++i
    ) {

        const char c =
            s[i];


        if (
            escape
        ) {

            escape = false;

            continue;

        }


        if (
            (single || dbl) &&
            c == '\\'
        ) {

            escape = true;

            continue;

        }


        if (
            c == '\'' &&
            !dbl
        ) {

            single = !single;

        }

        else if (
            c == '"' &&
            !single
        ) {

            dbl = !dbl;

        }

        else if (
            c == ';' &&
            !single &&
            !dbl
        ) {

            return i;

        }

    }


    return std::string::npos;

}


std::string timestamp(
    uint64_t t
) {

    return formatTimestamp(
        t
    );

}


std::string floatString(
    float x
) {

    std::ostringstream s;


    s
        << std::fixed
        << std::setprecision(4)
        << x;


    return s.str();

}


std::string millisString(
    double x
) {

    std::ostringstream s;


    s
        << std::fixed
        << std::setprecision(3)
        << x;


    return s.str();

}


term::Color tokenColor(
    TokenType t
) {

    switch (
        t
    ) {

        case TokenType::IDENTIFIER:

            return term::GREEN;


        case TokenType::INTEGER_LITERAL:

        case TokenType::FLOAT_LITERAL:

        case TokenType::STRING_LITERAL:

            return term::YELLOW;


        case TokenType::UNKNOWN:

            return term::RED;


        case TokenType::END_OF_FILE:

            return term::DIM;


        default:

            return term::CYAN;

    }

}


void printTopic(
    const std::string& title,
    const std::vector<std::string>& lines
) {

    std::cout
        << term::paint(
               title,
               {
                   term::BOLD,
                   term::CYAN
               }
           )
        << '\n';


    for (
        const std::string& line :
        lines
    ) {

        std::cout
            << line
            << '\n';

    }

}


bool showHelpTopic(
    const std::string& arg
) {

    std::string topic =
        upper(
            collapseSpaces(
                arg
            )
        );


    if (
        !topic.empty() &&
        topic.front() == '.'
    ) {

        topic.erase(
            topic.begin()
        );

    }


    if (
        topic == "HELP"
    ) {

        printTopic(
            ".help",
            {
                "Syntax:",
                "  .help",
                "  .help <cmd>",
                "",
                "Examples:",
                "  .help SELECT",
                "  .help CREATE TABLE",
                "  .help WHERE"
            }
        );

        return true;

    }


    if (
        topic == "CLEAR" ||
        topic == "CLS"
    ) {

        printTopic(
            ".clear",
            {
                "Syntax:",
                "  .clear",
                "  .cls",
                "",
                "Clears the REPL screen. No semicolon is needed."
            }
        );

        return true;

    }


    if (
        topic == "TOKENS"
    ) {

        printTopic(
            ".tokens",
            {
                "Syntax:",
                "  .tokens <statement>",
                "",
                "Example:",
                "  .tokens SELECT * FROM notes;"
            }
        );

        return true;

    }


    if (
        topic == "HISTORY"
    ) {

        printTopic(
            ".history / HISTORY",
            {
                "Meta-command syntax:",
                "  .history",
                "",
                "SQL syntax:",
                "  HISTORY <table> WHERE <condition>;",
                "",
                "Example:",
                "  HISTORY notes WHERE id = 1;"
            }
        );

        return true;

    }


    if (
        topic == "EXIT" ||
        topic == "QUIT"
    ) {

        printTopic(
            "exit",
            {
                "Syntax:",
                "  .exit",
                "  .quit",
                "  .q",
                "  exit",
                "  quit"
            }
        );

        return true;

    }


    if (
        topic == "CREATE" ||
        topic == "CREATE TABLE"
    ) {

        printTopic(
            "CREATE TABLE",
            {
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
            }
        );

        return true;

    }


    if (
        topic == "DROP" ||
        topic == "DROP TABLE"
    ) {

        printTopic(
            "DROP TABLE",
            {
                "Syntax:",
                "  DROP TABLE <table>;",
                "",
                "Example:",
                "  DROP TABLE notes;"
            }
        );

        return true;

    }


    if (
        topic == "DESCRIBE" ||
        topic == "DESCRIBE TABLE"
    ) {

        printTopic(
            "DESCRIBE TABLE",
            {
                "Syntax:",
                "  DESCRIBE TABLE <table>;",
                "",
                "Example:",
                "  DESCRIBE TABLE notes;"
            }
        );

        return true;

    }


    if (
        topic == "SHOW" ||
        topic == "SHOW TABLES"
    ) {

        printTopic(
            "SHOW TABLES",
            {
                "Syntax:",
                "  SHOW TABLES;"
            }
        );

        return true;

    }


    if (
        topic == "INSERT" ||
        topic == "INSERT INTO"
    ) {

        printTopic(
            "INSERT",
            {
                "Syntax:",
                "  INSERT INTO <table> VALUES (<v1>, <v2>, ...);",
                "",
                "Notes:",
                "  Values are positional and must match the table column order.",
                "  INSERT always creates a new version of the row.",
                "",
                "Example:",
                "  INSERT INTO notes VALUES (1, 'release notes');"
            }
        );

        return true;

    }


    if (
        topic == "UPDATE"
    ) {

        printTopic(
            "UPDATE",
            {
                "Syntax:",
                "  UPDATE <table> SET <col1> = <value1>[, <col2> = <value2> ...] [WHERE <condition>];",
                "",
                "Notes:",
                "  Without WHERE, every current row is updated.",
                "  The primary-key column cannot be updated.",
                "",
                "Example:",
                "  UPDATE notes SET body = 'fixed text' WHERE id = 1;"
            }
        );

        return true;

    }


    if (
        topic == "DELETE" ||
        topic == "DELETE FROM"
    ) {

        printTopic(
            "DELETE",
            {
                "Syntax:",
                "  DELETE FROM <table> [WHERE <condition>];",
                "",
                "Notes:",
                "  Without WHERE, every current row is deleted.",
                "",
                "Example:",
                "  DELETE FROM notes WHERE id = 1;"
            }
        );

        return true;

    }


    if (
        topic == "SELECT"
    ) {

        printTopic(
            "SELECT",
            {
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
            }
        );

        return true;

    }


    if (
        topic == "WHERE"
    ) {

        printTopic(
            "WHERE",
            {
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
            }
        );

        return true;

    }


    if (
        topic == "SIMILAR" ||
        topic == "SIMILAR TO" ||
        topic == "SEMANTIC"
    ) {

        printTopic(
            "SEMANTIC SEARCH",
            {
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
            }
        );

        return true;

    }


    if (
        topic == "AS OF" ||
        topic == "SNAPSHOT" ||
        topic == "BETWEEN" ||
        topic == "TEMPORAL"
    ) {

        printTopic(
            "TEMPORAL SELECT",
            {
                "Syntax:",
                "  SELECT * FROM <table> AS OF <date> [WHERE <condition>];",
                "  SELECT * FROM <table> SNAPSHOT <date> [WHERE <condition>];",
                "  SELECT * FROM <table> BETWEEN <date1> AND <date2> [WHERE <condition>];",
                "",
                "Notes:",
                "  AS OF and SNAPSHOT return a snapshot at that date or instant.",
                "  BETWEEN returns versions in the date range.",
                "  WHERE can appear before or after the temporal clause."
            }
        );

        return true;

    }


    if (
        topic == "COMPARE"
    ) {

        printTopic(
            "COMPARE",
            {
                "Syntax:",
                "  COMPARE <table> WHERE <condition> BETWEEN <date1> AND <date2>;",
                "",
                "Example:",
                "  COMPARE notes WHERE id = 1 BETWEEN 2026-09-01 AND 2026-09-20;"
            }
        );

        return true;

    }


    if (
        topic == "EVOLUTION"
    ) {

        printTopic(
            "EVOLUTION",
            {
                "Syntax:",
                "  EVOLUTION <table> WHERE <condition> BETWEEN <date1> AND <date2>;",
                "",
                "Example:",
                "  EVOLUTION notes WHERE id = 1 BETWEEN 2026-09-01 AND 2026-09-20;"
            }
        );

        return true;

    }


    if (
        topic == "ROLLBACK"
    ) {

        printTopic(
            "ROLLBACK",
            {
                "Syntax:",
                "  ROLLBACK <table> WHERE <condition> TO <date>;",
                "  ROLLBACK TABLE <table> TO <date>;",
                "",
                "Examples:",
                "  ROLLBACK notes WHERE id = 1 TO 2026-09-20;",
                "  ROLLBACK TABLE notes TO 2026-09-20;"
            }
        );

        return true;

    }


    if (
        topic == "COMPACT" ||
        topic == "COMPACT TABLE"
    ) {

        printTopic(
            "COMPACT TABLE",
            {
                "Syntax:",
                "  COMPACT TABLE <table> TO <date>;",
                "",
                "Example:",
                "  COMPACT TABLE notes TO 2026-09-20;"
            }
        );

        return true;

    }


    if (
        topic == "DATE" ||
        topic == "TIME" ||
        topic == "TIMESTAMP"
    ) {

        printTopic(
            "DATE / TIMESTAMP",
            {
                "Supported forms:",
                "  YYYY-MM-DD",
                "  YYYY-MM-DD HH",
                "  YYYY-MM-DD HH:MM",
                "  YYYY-MM-DD HH:MM:SS",
                "  YYYY-MM-DD HH:MM:SS.mmm",
                "",
                "Example:",
                "  2026-09-20 14:30:45.250"
            }
        );

        return true;

    }


    if (
        topic == "ORDER" ||
        topic == "ORDER BY"
    ) {

        printTopic(
            "ORDER BY",
            {
                "Syntax:",
                "  SELECT ... ORDER BY <col> [ASC|DESC];",
                "",
                "Notes:",
                "  ORDER BY is supported in SELECT.",
                "  ASC is the default when ASC/DESC is omitted.",
                "",
                "Example:",
                "  SELECT * FROM notes ORDER BY id DESC LIMIT 5;"
            }
        );

        return true;

    }


    if (
        topic == "LIMIT"
    ) {

        printTopic(
            "LIMIT",
            {
                "Syntax:",
                "  SELECT ... LIMIT <n>;",
                "",
                "Notes:",
                "  LIMIT is supported in SELECT.",
                "  Semantic search defaults to 10 results when LIMIT is omitted.",
                "",
                "Example:",
                "  SELECT * FROM notes WHERE body SIMILAR TO 'search text' LIMIT 5;"
            }
        );

        return true;

    }


    if (
        topic == "TYPE" ||
        topic == "TYPES" ||
        topic == "DATA TYPES"
    ) {

        printTopic(
            "DATA TYPES",
            {
                "Supported types:",
                "  INT",
                "  FLOAT",
                "  BOOL",
                "  STRING(<size>)",
                "",
                "Column modifiers:",
                "  PRIMARY KEY",
                "  SEMANTIC"
            }
        );

        return true;

    }


    return false;

}


}


Repl::Repl()
    : executor(catalog) {

    term::init();


    term::setTitle(
        "MemoraDB"
    );

    embedder =
        std::make_unique<MiniLmEmbedder>(
            getModelDirectory().string()
        );


    executor.setEmbeddingProvider(
        [this](
            const std::string& text,
            float (&out)[VEC_DIM]
        ) {

            try {

                const auto e =
                    embedder->encode(text);


                if (
                    e.size() != VEC_DIM
                ) {

                    return false;

                }


                std::copy(
                    e.begin(),
                    e.end(),
                    out
                );


                return true;

            }

            catch (
                const std::exception& error
            ) {

                std::cerr
                    << "ONNX embedding failed: "
                    << error.what()
                    << '\n';


                return false;

            }

        }
    );

}


namespace {

struct Rgb {
    int r;
    int g;
    int b;
};

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

Rgb mixRgb(const Rgb& a, const Rgb& b, double t) {

    return Rgb{
        static_cast<int>(a.r + (b.r - a.r) * t),
        static_cast<int>(a.g + (b.g - a.g) * t),
        static_cast<int>(a.b + (b.b - a.b) * t)
    };

}

Rgb bannerGradient(double t) {

    static const Rgb stops[4] = {
        {0, 229, 255},
        {99, 102, 241},
        {192, 72, 255},
        {255, 84, 160}
    };

    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;

    const double scaled = t * 3.0;
    int i = static_cast<int>(scaled);
    if (i > 2) i = 2;

    return mixRgb(stops[i], stops[i + 1], scaled - i);

}


bool supportsTrueColor() {

    static const bool enabled = [] {

#ifdef _WIN32
        return true;
#else
        const char* ct = std::getenv("COLORTERM");

        if (ct) {
            const std::string v(ct);
            if (v == "truecolor" || v == "24bit") return true;
        }

        return false;
#endif

    }();

    return enabled;

}


int toAnsi256(const Rgb& c) {

    auto level = [](int v) {
        return static_cast<int>(v / 255.0 * 5.0 + 0.5);
    };

    return 16 + 36 * level(c.r) + 6 * level(c.g) + level(c.b);

}


std::string rgbEscape(const Rgb& c, bool bold) {

    const std::string prefix =
        std::string("\033[") + (bold ? "1;" : "");

    if (supportsTrueColor()) {

        return prefix
               + "38;2;"
               + std::to_string(c.r) + ";"
               + std::to_string(c.g) + ";"
               + std::to_string(c.b) + "m";

    }

    return prefix + "38;5;" + std::to_string(toAnsi256(c)) + "m";

}


std::string rgbPaint(const std::string& text, const Rgb& c, bool bold = false) {

    if (!term::colorsEnabled()) return text;

    return rgbEscape(c, bold) + text + "\033[0m";

}


std::string gradientLine(
    const std::string& line,
    int row,
    int rows,
    int totalWidth
) {

    if (!term::colorsEnabled()) return line;

    const double span =
        static_cast<double>(totalWidth + rows * 3);

    std::string out;

    for (size_t i = 0; i < line.size(); ++i) {

        const char ch = line[i];

        if (ch == ' ') {
            out += ch;
            continue;
        }

        const double t =
            (static_cast<double>(i) + row * 3.0) / span;

        out += rgbEscape(bannerGradient(t), true);
        out += ch;

    }

    return out + "\033[0m";

}

std::string gradientText(const std::string& text) {

    if (!term::colorsEnabled()) return text;

    std::string out;
    const double span =
        text.size() > 1 ? static_cast<double>(text.size() - 1) : 1.0;

    for (size_t i = 0; i < text.size(); ++i) {

        if (text[i] == ' ') {
            out += ' ';
            continue;
        }

        out += rgbEscape(bannerGradient(i / span), true);
        out += text[i];

    }

    return out + "\033[0m";

}


std::string gradientRule(const std::string& glyph, int width) {

    if (width < 1) return "";

    std::string out;

    for (int i = 0; i < width; ++i) {

        if (term::colorsEnabled()) {
            const double t =
                width > 1 ? static_cast<double>(i) / (width - 1) : 0.0;
            out += rgbEscape(bannerGradient(t), true);
        }

        out += glyph;

    }

    if (term::colorsEnabled()) out += "\033[0m";

    return out;

}

}


void Repl::banner() {

    const std::string dot = "\342\227\217";   
    const std::string bar = "\342\224\200";   
    const int cols = term::width();
    const int indent = 2;
    const std::string pad(indent, ' ');

    std::cout << '\n';


    

    if (cols >= indent + kLogoMemoraWidth + 2 + kLogoDbWidth + 2) {

        
        const int total = kLogoMemoraWidth + 2 + kLogoDbWidth;

        for (int r = 0; r < 8; ++r) {

            std::cout
                << pad
                << gradientLine(
                       std::string(kLogoMemora[r]) + "  " + kLogoDb[r],
                       r,
                       8,
                       total
                   )
                << '\n';

        }

    }
    else if (cols >= indent + kLogoMemoraWidth + 2) {

        
        for (int r = 0; r < 8; ++r) {

            std::cout
                << pad
                << gradientLine(
                       kLogoMemora[r],
                       r,
                       16,
                       kLogoMemoraWidth
                   )
                << '\n';

        }

        const std::string dbPad(
            static_cast<size_t>((kLogoMemoraWidth - kLogoDbWidth) / 2),
            ' '
        );

        for (int r = 0; r < 8; ++r) {

            std::cout
                << pad
                << dbPad
                << gradientLine(
                       kLogoDb[r],
                       r + 8,
                       16,
                       kLogoMemoraWidth
                   )
                << '\n';

        }

    }
    else {

        std::cout
            << pad
            << gradientText("M E M O R A   D B")
            << '\n';

    }

    std::cout << '\n'; 

    int ruleWidth = cols - indent * 2;
    if (ruleWidth > kLogoMemoraWidth) ruleWidth = kLogoMemoraWidth;
    if (ruleWidth < 20) ruleWidth = 20;

    std::cout
        << pad
        << rgbPaint("semantic + temporal dbms", Rgb{255, 214, 102}, true)
        << '\n'
        << pad
        << gradientRule(bar, ruleWidth)
        << "\n\n"; 

    auto led =
        [&](
            const std::string& label,
            bool ok
        ) {

            std::string cell =
                ok
                    ? term::paint(dot, {term::BOLD, term::GREEN})
                    : term::paint(dot, {term::BOLD, term::RED});

            cell += "  ";
            cell += ok
                        ? label
                        : term::paint(label + " (off)", term::GRAY);

            return cell;

        };

    auto cellPad =
        [](
            const std::string& label,
            bool ok
        ) {

            
            const size_t visible =
                3 + label.size() + (ok ? 0 : 6);

            const size_t target = 36;

            return std::string(
                visible < target
                    ? target - visible
                    : 2,
                ' '
            );

        };

    const bool semanticOk = embedder != nullptr;

    const std::string l1 = "temporal engine ready";
    const std::string l2 = "append-only storage ready";
    const std::string l3 = "semantic vector search ready";
    const std::string l4 = "embedding model ready";

    if (cols >= 76) {

        std::cout
            << pad << led(l1, true) << cellPad(l1, true) << led(l2, true)
            << '\n'
            << pad << led(l3, semanticOk) << cellPad(l3, semanticOk) << led(l4, semanticOk)
            << "\n\n";

    }
    else {

        
        std::cout
            << pad << led(l1, true) << '\n'
            << pad << led(l2, true) << '\n'
            << pad << led(l3, semanticOk) << '\n'
            << pad << led(l4, semanticOk)
            << "\n\n";

    }

    std::cout
        << pad
        << term::paint(
               "MemoraDB is a C++ append-only temporal database built from scratch featuring",
               term::GRAY
           )
        << '\n'
        << pad
        << term::paint(
               "a custom binary storage engine, schema serialization, immutable versioned records,",
               term::GRAY
           )
        << '\n'
        << pad
        << term::paint(
               "crash recovery, temporal indexing, snapshots, rollback, time-travel queries,",
               term::GRAY
           )
        << '\n'
        << pad
        << term::paint(
               "semantic vector search, and a SQL-like query engine.",
               term::GRAY
           )
        << '\n'
        << pad
        << term::paint(
               "GitHub Link: ",
               term::GREEN
           )
        << '\n'
        << pad
        << term::paint(
               "https://github.com/v1haan24/MemoraDB.git",
               term::WHITE
           )
        << "\n\n"
        << pad
        << term::paint("Type ", term::GRAY)
        << rgbPaint(".help", Rgb{0, 229, 255}, true)
        << term::paint(" for commands, ", term::GRAY)
        << rgbPaint(".about", Rgb{192, 72, 255}, true)
        << term::paint(" for info, ", term::GRAY)
        << rgbPaint(".exit", Rgb{255, 84, 160}, true)
        << term::paint(" to quit.", term::GRAY)
        << "\n\n";

}


void Repl::help() {

    std::cout
        << term::paint(
               "Commands",
               {
                   term::BOLD,
                   term::CYAN
               }
           )
        << '\n'

        << "  .help [cmd]        show help, or syntax for a command\n"

        << "  .tokens <stmt>     show lexer tokens\n"

        << "  .history           show command history\n"

        << "  .clear             clear screen\n"

        << "  .about             about MemoraDB\n"

        << "  .exit/.quit/.q     exit shell\n\n"

        << "SQL may span lines; terminate statements with ';'.\n"
        << "Try .help SELECT, .help CREATE TABLE, .help WHERE, or .help DATE.\n";

}


void Repl::about() {

    std::cout
        << term::paint(
               "MemoraDB -",
               {
                   term::BOLD,
                   term::MAGENTA
               }
           )
        <<"\n"
        << term::paint(
               "A modular C++ append-only temporal database built from scratch featuring",
               term::GRAY
           )
        << '\n'
        << term::paint(
               "a custom binary storage engine, schema serialization, immutable versioned records,",
               term::GRAY
           )
        << '\n'
        << term::paint(
               "crash recovery, temporal indexing, snapshots, rollback, time-travel queries,",
               term::GRAY
           )
        << '\n'
        << term::paint(
               "semantic vector search, and a SQL-like query engine.",
               term::GRAY
           )
        << '\n'
        << term::paint(
               "GitHub Link: ",
               term::GREEN
           )
        << '\n'
        << term::paint(
               "https://github.com/v1haan24/MemoraDB.git\n",
               term::WHITE
           );
}


void Repl::showTokens(
    const std::string& sql
) {

    const auto start =
        std::chrono::steady_clock::now();


    Lexer lexer(sql);


    const auto ts =
        lexer.tokenize();


    std::cout
        << std::left
        << std::setw(5)
        << '#'
        << std::setw(20)
        << "type"
        << std::setw(10)
        << "line:col"
        << "value\n";


    for (
        size_t i = 0;
        i < ts.size();
        ++i
    ) {

        const auto& t =
            ts[i];


        std::cout
            << std::left
            << std::setw(5)
            << i

            << std::setw(20)
            << term::paint(
                   tokenTypeToString(t.type),
                   tokenColor(t.type)
               )

            << std::setw(10)
            << std::to_string(t.line)
            + ':'
            + std::to_string(t.column)

            << t.value
            << '\n';


        if (
            t.type ==
            TokenType::END_OF_FILE
        ) {

            break;

        }

    }


    const auto ms =
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now()
            - start
        ).count();


    std::cout
        << "Tokens: "
        << ts.size()
        << " ("
        << std::fixed
        << std::setprecision(3)
        << ms
        << " ms)\n";

}


void Repl::dispatchMeta(
    const std::string& command
) {

    std::istringstream in(
        command
    );


    std::string cmd;
    std::string arg;


    in >> cmd;


    std::getline(
        in,
        arg
    );


    arg =
        trim(arg);


    const auto c =
        lower(cmd);


    if (
        c == ".help" ||
        c == ".h"
    ) {

        if (
            arg.empty()
        ) {

            help();

        }

        else if (
            !showHelpTopic(
                arg
            )
        ) {

            std::cout
                << term::paint(
                       "No help topic for: ",
                       term::YELLOW
                   )
                << arg
                << '\n'
                << "Available topics: CREATE, DROP, DESCRIBE, SHOW, INSERT, UPDATE, DELETE, SELECT, WHERE, SIMILAR, AS OF, ORDER BY, LIMIT, COMPARE, EVOLUTION, HISTORY, ROLLBACK, COMPACT, DATE, TYPES, CLEAR.\n";

        }

    }


    else if (
        c == ".about"
    ) {

        about();

    }


    else if (
        c == ".tokens"
    ) {

        if (
            arg.empty()
        ) {

            std::cout
                << "usage: .tokens <statement>\n";

        }

        else {

            showTokens(arg);

        }

    }


    else if (
        c == ".history"
    ) {

        if (
            history.empty()
        ) {

            std::cout
                << "(empty)\n";

        }


        for (
            size_t i = 0;
            i < history.size();
            ++i
        ) {

            std::cout
                << i + 1
                << "  "
                << history[i]
                << '\n';

        }

    }


    else if (
        c == ".clear" ||
        c == ".cls"
    ) {

        term::clear();

    }


    else if (
        c == ".exit" ||
        c == ".quit" ||
        c == ".q" ||
        c == "exit" ||
        c == "quit"
    ) {

        running = false;

    }


    else {

        std::cout
            << term::paint(
                   "Unknown command: ",
                   term::RED
               )
            << cmd
            << " (try .help)\n";

    }

}


bool Repl::readStatement(
    std::string& statement,
    std::string& meta
) {

    statement.clear();


    meta.clear();


    std::string buffer =
        pending;


    pending.clear();


    while (
        running
    ) {

        std::cout
            << term::paint(
                   buffer.empty()
                       ? "memora> "
                       : "   ...> ",
                   buffer.empty()
                       ? std::initializer_list<term::Color>{
                             term::BOLD,
                             term::MAGENTA
                         }
                       : std::initializer_list<term::Color>{
                             term::DIM
                         }
               )
            << std::flush;


        std::string line;


        if (
            !std::getline(
                std::cin,
                line
            )
        ) {

            if (
                interrupted
            ) {

                interrupted = 0;


                buffer.clear();


                std::cin.clear();


                continue;

            }


            running = false;


            return false;

        }


        if (
            buffer.empty() &&
            trim(line).empty()
        ) {

            continue;

        }


        if (
            buffer.empty() &&
            line[0] == '.'
        ) {

            meta =
                trim(line);


            return true;

        }


        if (
            !buffer.empty()
        ) {

            buffer += '\n';

        }


        buffer += line;


        const auto pos =
            findTerminator(
                buffer
            );


        if (
            pos == std::string::npos
        ) {

            continue;

        }


        statement =
            trim(
                buffer.substr(
                    0,
                    pos
                )
            );


        pending =
            trim(
                buffer.substr(
                    pos + 1
                )
            );


        return !statement.empty();

    }


    return false;

}


void Repl::printResult(
    const ExecResult& r
) {

    if (
        !r.ok()
    ) {

        std::cout
            << term::paint(
                   "Error: ",
                   {
                       term::BOLD,
                       term::RED
                   }
               )
            << r.message
            << '\n';


        return;

    }


    if (
        r.kind ==
        ExecResult::Kind::OK
    ) {

        std::cout
            << term::paint(
                   "OK ",
                   {
                       term::BOLD,
                       term::GREEN
                   }
               )
            << term::paint(
                   r.message.empty()
                       ? "OK"
                       : r.message,
                   term::CYAN
               )
            << '\n';


        return;

    }


    std::vector<std::string> h;


    std::vector<
        std::vector<std::string>
    > rows;


    if (
        r.kind ==
        ExecResult::Kind::ROWS
    ) {

        for (
            const auto& c :
            r.columns
        ) {

            h.push_back(
                c.name
            );

        }


        h.insert(
            h.end(),
            {
                "_timestamp",
                "_deleted"
            }
        );


        for (
            const auto& x :
            r.records
        ) {

            auto row =
                x.row.values;


            row.push_back(
                timestamp(
                    x.timestamp
                )
            );


            row.push_back(
                x.deleted
                    ? "true"
                    : "false"
            );


            rows.push_back(
                std::move(row)
            );

        }


        if (
            rows.empty()
        ) {

            std::cout
                << term::paint(
                       "No rows returned",
                       term::YELLOW
                   )
                << '\n';


            return;

        }

    }


    else if (
        r.kind ==
        ExecResult::Kind::DIFFS
    ) {

        h = {
            "timestamp",
            "column",
            "before",
            "after"
        };


        for (
            const auto& x :
            r.diffs
        ) {

            rows.push_back(
                {
                    timestamp(x.timestamp),
                    x.column,
                    x.before,
                    x.after
                }
            );

        }


        if (
            rows.empty()
        ) {

            std::cout
                << term::paint(
                       "No differences found",
                       term::YELLOW
                   )
                << '\n';


            return;

        }

    }


    else {

        h = {
            "pk",
            "timestamp",
            "score"
        };

        for (
            const auto& c :
            r.columns
        ) {

            h.push_back(
                c.name
            );

        }


        for (
            const auto& x :
            r.search
        ) {

            rows.push_back(
                {
                    x.pk,
                    timestamp(x.timestamp),
                    floatString(x.score)
                }
            );

            for (
                const auto& value :
                x.semanticValues
            ) {

                rows.back().push_back(
                    value
                );

            }

        }


        if (
            rows.empty()
        ) {

            std::cout
                << term::paint(
                       "No semantic matches found",
                       term::YELLOW
                   )
                << '\n';


            return;

        }

    }


    term::printTable(
        h,
        rows
    );


    if (
        !r.message.empty()
    ) {

        std::cout
            << term::paint(
                   r.message,
                   term::LIGHT_YELLOW
               )
            << '\n';

    }

}


void Repl::executeProgram(
    const std::string& input
) {

    try {

        Lexer lexer(
            input
        );


        Parser parser(
            lexer.tokenize()
        );


        for (
            const auto& statement :
            parser.parseProgram()
        ) {

            const auto start =
                std::chrono::steady_clock::now();


            ExecResult result =
                executor.execute(
                    statement
                );


            const auto ms =
                std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now()
                    - start
                ).count();


            printResult(
                result
            );


            std::cout
                << term::paint(
                       "Executed in " +
                       millisString(
                           ms
                       ) +
                       " ms",
                       term::LIGHT_YELLOW
                   )
                << '\n';


            if (
                !result.ok()
            ) {

                break;

            }

        }

    }


    catch (
        const ParseError& e
    ) {

        std::cout
            << term::paint(
                   "Parse Error: ",
                   {
                       term::BOLD,
                       term::RED
                   }
               )
            << e.what()
            << '\n'

            << term::paint(
                   "line " +
                   std::to_string(e.line) +
                   ", column " +
                   std::to_string(e.column),
                   term::GRAY
               )
            << '\n';

    }


    catch (
        const std::exception& e
    ) {

        std::cout
            << term::paint(
                   "Error: ",
                   {
                       term::BOLD,
                       term::RED
                   }
               )
            << e.what()
            << '\n';

    }

}


void Repl::run() {

    banner();


    std::signal(
        SIGINT,
        handleInterrupt
    );


    while (
        running
    ) {

        if (
            interrupted
        ) {

            interrupted = 0;


            pending.clear();


            std::cout
                << '\n'
                << term::paint(
                       "Query cancelled.",
                       term::YELLOW
                   )
                << '\n';


            continue;

        }


        std::string sql;
        std::string meta;


        if (
            !readStatement(
                sql,
                meta
            )
        ) {

            break;

        }


        if (
            !meta.empty()
        ) {

            dispatchMeta(
                meta
            );

        }


        else if (
            !sql.empty()
        ) {

            history.push_back(
                sql
            );


            executeProgram(
                sql + ';'
            );

        }

    }


    std::cout
        << term::paint(
               "Goodbye.",
               term::DIM
           )
        << '\n';

}


int Repl::runCommandLine(
    int argc,
    char** argv
) {

    if (
        argc == 1
    ) {

        run();


        return 0;

    }


    std::ostringstream in;


    for (
        int i = 1;
        i < argc;
        ++i
    ) {

        if (
            i > 1
        ) {

            in << ' ';

        }


        in << argv[i];

    }


    const auto input =
        trim(
            in.str()
        );


    if (
        input.empty()
    ) {

        return 0;

    }


    if (
        input[0] == '.'
    ) {

        dispatchMeta(
            input
        );


        return 0;

    }


    executeProgram(
        input.back() == ';'
            ? input
            : input + ';'
    );


    return 0;

}


int main(
    int argc,
    char** argv
) {

    return Repl().runCommandLine(
        argc,
        argv
    );

}