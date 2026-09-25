# Testing

MemoraDB includes a comprehensive automated test suite built with **GoogleTest (GTest)** and **CTest**, as well as manual end-to-end verification workflows.

---

## Automated Test Suite Architecture

The automated test suite is located in the `test/` directory and is decoupled from normal builds via the CMake option `MEMORA_BUILD_TESTS`:

* **No Production Overhead**: During a standard build (`cmake -S . -B build`), the test framework is not downloaded and test targets are not compiled.
* **Hermetic Filesystem Isolation**: Database tests instantiate the RAII helper `TempDirectory` (from `test/test_helper.h`). Each test executes in a freshly generated temporary directory and cleans up on teardown. Your local `data/` folder is never touched.
* **Zero External Network Dependencies**: Semantic vector tests do not require downloading ONNX model weights or Hugging Face assets. Tests inject deterministic mock lambdas using `Executor::setEmbeddingProvider(...)`, guaranteeing fast and offline test execution.

---

## Test Suites Overview

The automated suite consists of 47 tests across 9 specialized test modules:

| Test Module | Source File | Subsystem Under Test | Key Scenarios Covered |
|---|---|---|---|
| **Lexer** | `test/lexer/test_lexer.cpp` | Lexical Analysis | All 33 keywords (case-insensitivity), identifiers, integer/float literals, string escapes, operators (`=`, `!=`, `<`, `<=`, `>`, `>=`), line/column tracking, unterminated strings. |
| **Parser** | `test/parser/test_parser.cpp` | Abstract Syntax Tree | `CREATE TABLE`, `DROP`, `DESCRIBE`, `SHOW`, `INSERT`, `UPDATE`, `DELETE`, `SELECT` (projections, `WHERE`, `ORDER BY`, `LIMIT`), temporal clauses (`AS OF`, `BETWEEN`, `SNAPSHOT`), semantic clauses (`SIMILAR TO`), diffing statements (`COMPARE`, `EVOLUTION`, `HISTORY`, `ROLLBACK`, `COMPACT`), and `ParseError` diagnostics. |
| **Catalog** | `test/catalog/test_catalog.cpp` | Catalog & Schema Manager | Schema validation, single primary key enforcement, duplicate column name rejection, table dropping, and disk reload persistence. |
| **Storage Engine** | `test/storage/test_storage.cpp` | Binary Storage Engine | Column type validation (INT, FLOAT, STRING length, BOOL), append-only records, tombstone deletion, reading past records, and `compareRecords` diff calculation. |
| **Query & Executor** | `test/query/test_executor.cpp` | Query Execution Engine | DDL/DML statement execution, multi-row filtering, ascending/descending sorting, limit slicing, projection mapping, and error reporting. |
| **Temporal Engine** | `test/temporal/test_temporal.cpp` | Temporal Subsystem | `showHistory`, point-in-time `selectAsOf`, time interval `selectBetween`, `snapshot`, field-level `evolution`, `compare` diffs, and row rollback. |
| **History Index** | `test/index/test_index.cpp` | In-Memory Index | Version tracking, binary-search point-in-time resolution (`latestBefore`), and key enumeration. |
| **Vector Engine** | `test/vector/test_vector.cpp` | Semantic & Vector Engine | 384-dimensional `cosineSimilarity` calculations (identical, orthogonal, opposite, zero), and mock-based end-to-end `SIMILAR TO` query ranking. |
| **Integration** | `test/integration/test_integration.cpp` | Multi-Subsystem Workflows | Multi-statement database lifecycle: table creation with semantic columns, updates, historical querying, catalog restart persistence across instances, and rollback. |

---

## Building and Running Tests

### 1. Build the Test Suite

Configure a dedicated build directory with `-DMEMORA_BUILD_TESTS=ON`:

=== "With Ninja (Recommended)"

    ```powershell
    # Configure and build
    cmake -G "Ninja" -S . -B build-tests -DMEMORA_BUILD_TESTS=ON
    cmake --build build-tests
    ```

=== "Without Ninja (MinGW Makefiles)"

    ```powershell
    # Configure and build
    cmake -G "MinGW Makefiles" -S . -B build-tests -DMEMORA_BUILD_TESTS=ON
    cmake --build build-tests
    ```

=== "Linux / macOS"

    ```bash
    cmake -G "Ninja" -S . -B build-tests -DMEMORA_BUILD_TESTS=ON
    cmake --build build-tests
    ```

---

### 2. Run All Tests via CTest

Execute the full suite and display failure diagnostics:

```bash
ctest --test-dir build-tests --output-on-failure
```

Expected output:
```text
100% tests passed out of 47
Total Test time (real) = 2.11 sec
```

---

### 3. Run Specific Test Suites via GTest Filters

You can execute the compiled test binary directly and filter specific suites or tests using `--gtest_filter`:

```bash
# Run only Temporal engine tests:
./build-tests/test/memora_tests --gtest_filter=TemporalTest.*

# Run only Parser tests:
./build-tests/test/memora_tests --gtest_filter=ParserTest.*

# Run only Vector and Semantic search tests:
./build-tests/test/memora_tests --gtest_filter=VectorTest.*

# Run a single specific test:
./build-tests/test/memora_tests --gtest_filter=IntegrationTest.FullWorkflowWithPersistence
```

---

## Writing New Tests

When adding a new feature or fixing a bug, add unit tests in the appropriate `test/` subdirectory:

1. **Use `TempDirectory` for Storage**: Any test that creates tables or writes files must declare a `TempDirectory` instance as its first local variable:
   ```cpp
   TEST(MySuite, MyTest) {
       TempDirectory tempDir;
       Catalog catalog;
       // Any data written here will be automatically erased on teardown!
   }
   ```
2. **Mock Embeddings for Semantic Code**: Use `Executor::setEmbeddingProvider` with a lambda instead of loading the real ONNX model:
   ```cpp
   executor.setEmbeddingProvider([](const std::string& text, float (&out)[VEC_DIM]) -> bool {
       std::fill(std::begin(out), std::end(out), 0.0f);
       if (text.find("query") != std::string::npos) out[0] = 1.0f;
       return true;
   });
   ```
3. **Register in `test/CMakeLists.txt`**: Add any new `.cpp` test file to the `TEST_SOURCES` list in `test/CMakeLists.txt`.

---

## Manual Verification (`commands/MemoraDB-test-script.md`)

In addition to automated tests, you can run an interactive end-to-end verification script inside the REPL using the statements in `commands/MemoraDB-test-script.md` to visually inspect terminal colors, diagnostics, and table outputs.
