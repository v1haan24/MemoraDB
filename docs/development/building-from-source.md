# Building from Source

This guide covers the build system, compiler configurations, and compilation steps for building MemoraDB across supported platforms.

---

## Build System Architecture

MemoraDB uses **CMake 3.20+** as its meta-build system. The build configuration is defined in the root `CMakeLists.txt` and manages:

* **Language Standard**: ISO C++17 (`set(CMAKE_CXX_STANDARD 17)`).
* **Automatic Dependency Fetching**: Uses `FetchContent` to download Microsoft ONNX Runtime 1.22.0.
* **Automatic Model Acquisition**: Downloads `model.onnx` and `vocab.txt` from Hugging Face (`sentence-transformers/all-MiniLM-L6-v2`) if not already present.
* **Compilation Targets**:
  * `memora` (Static Library): Compiles all DBMS subsystems (catalog, storage, index, temporal, vector, query, lexer, parser, engine).
  * `memora_cli` (Executable): Compiles the terminal REPL and links against `memora` (output name set to `memora`).
  * `memora_tests` (Test Executable): Optional test target built when `-DMEMORA_BUILD_TESTS=ON`.
* **Post-Build Automation**: Copies dynamic libraries (`onnxruntime.dll`) and model assets to the output directory adjacent to the binary.

---

## Prerequisites

Before building, ensure you have:
* A modern **C++17** compiler:
  * Windows: Visual Studio 2022 (MSVC) or MinGW-w64 (GCC 9+)
  * Linux: GCC 9+ or Clang 10+
  * macOS: Apple Clang 12+ (Xcode Command Line Tools)
* **CMake 3.20+**
* **Git**
* *(Recommended)* **Ninja** build tool for fast parallel builds

---

## Step-by-Step Build Instructions

### 1. Clone the Repository

```bash
git clone https://github.com/v1haan24/MemoraDB.git
cd MemoraDB
```

---

### 2. Windows

=== "With Ninja (Recommended)"

    Open PowerShell or an **x64 Native Tools Command Prompt for VS 2022**:

    ```powershell
    # Configure with Ninja
    cmake -G "Ninja" -S . -B build

    # Build Release target
    cmake --build build --config Release

    # Launch MemoraDB
    .\build\memora.exe
    ```

=== "Without Ninja (MinGW Makefiles)"

    If you have MinGW-w64 (`mingw32-make.exe` in PATH):

    ```powershell
    # Configure with MinGW Makefiles
    cmake -G "MinGW Makefiles" -S . -B build

    # Build Release target
    cmake --build build --config Release

    # Launch MemoraDB
    .\build\memora.exe
    ```

---

### 3. Linux (Ubuntu / Debian / Fedora / Arch)

=== "With Ninja (Recommended)"

    ```bash
    # Install dependencies on Ubuntu/Debian:
    # sudo apt-get update && sudo apt-get install -y cmake ninja-build build-essential git

    # Configure
    cmake -G "Ninja" -S . -B build -DCMAKE_BUILD_TYPE=Release

    # Compile
    cmake --build build

    # Run
    ./build/memora
    ```

=== "Without Ninja (Unix Makefiles)"

    ```bash
    # Configure with default Makefiles
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

    # Compile using all available CPU cores
    cmake --build build -j$(nproc)

    # Run
    ./build/memora
    ```

---

### 4. macOS (Apple Silicon or Intel)

```bash
# Install tools:
# brew install cmake ninja git

# Configure
cmake -G "Ninja" -S . -B build -DCMAKE_BUILD_TYPE=Release

# Compile
cmake --build build

# Run
./build/memora
```

---

## Building and Running the Test Suite

MemoraDB features an automated developer test suite using GoogleTest and CTest. Testing is disabled by default to keep user builds lightweight.

To compile and execute the test suite:

```bash
# 1. Configure in a separate build directory with tests enabled
cmake -G "Ninja" -S . -B build-tests -DMEMORA_BUILD_TESTS=ON

# 2. Compile tests
cmake --build build-tests

# 3. Run all tests via CTest
ctest --test-dir build-tests --output-on-failure

# 4. Or run the test executable directly (supports GTest filters)
./build-tests/test/memora_tests --gtest_filter=TemporalTest.*
```

For more details on test fixtures, isolation, and adding new test suites, see the [Testing Guide](testing.md).

---

## Offline / Air-Gapped Builds

If building in an environment without internet access:

1. **Pre-Download ONNX Runtime**:
   Download the archive for your platform and set the CMake cache variable:
   ```powershell
   cmake -S . -B build -G Ninja -DONNXRUNTIME_ROOT="C:/path/to/extracted/onnxruntime"
   ```
2. **Pre-Download Model Assets**:
   Download `model.onnx` and `vocab.txt` from Hugging Face and place them into `build/models/all-MiniLM-L6-v2/` before compiling.
