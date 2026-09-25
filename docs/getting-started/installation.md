# Installation & Setup

This guide explains how to install and run MemoraDB, either by downloading a pre-built release package or building from source.

---

## Pre-Built Binaries (Recommended for Users)

If you just want to run MemoraDB without compiling code:

1. Go to the [GitHub Releases](https://github.com/v1haan24/MemoraDB/releases) page.
2. Download the pre-packaged archive for your operating system:
   * **Windows**: `MemoraDB-windows-x64.zip`
   * **Linux**: `MemoraDB-linux-x64.tar.gz`
   * **macOS**: `MemoraDB-macos-universal.tar.gz`
3. Extract the downloaded archive to any folder on your machine.
4. Open your terminal in that folder and run the executable:
   * On Windows: `.\memora.exe`
   * On Linux/macOS: `./memora`

> [!TIP]
> **Zero Extra Setup Required**: All necessary runtime libraries (including ONNX Runtime) and the default neural embedding model are fully pre-packaged inside the release archive. You do not need to install Python, external vector databases, or any auxiliary services.

---

## Building from Source (For Developers)

Building MemoraDB from source gives you full access to the codebase, build flags, and developer test suite.

### 1. Install Prerequisites

Ensure you have a C++17 compatible compiler, Git, CMake, and Ninja installed:

=== "Windows"

    You can quickly install the required tools using `winget` in PowerShell:

    ```powershell
    # Install CMake and Ninja
    winget install --id Kitware.CMake -e
    winget install --id Ninja-build.Ninja -e
    ```

    For the C++17 compiler, you can use either:
    * **Visual Studio 2022** with the *"Desktop development with C++"* workload, or
    * **MinGW-w64** (e.g., via MSYS2: `pacman -S mingw-w64-ucrt-x86_64-gcc`).

=== "Linux (Ubuntu / Debian)"

    Install the build essentials, CMake, Git, and Ninja via `apt`:

    ```bash
    sudo apt update
    sudo apt install -y git cmake ninja-build build-essential
    ```

=== "macOS"

    Install tools using [Homebrew](https://brew.sh/):

    ```bash
    brew install git cmake ninja
    ```

---

### 2. Clone the Repository

Clone the MemoraDB repository from GitHub:

```bash
git clone https://github.com/v1haan24/MemoraDB.git
cd MemoraDB
```

---

### 3. Compile the Database

You can compile with Ninja (fastest) or without Ninja using standard Makefiles:

=== "With Ninja (Recommended)"

    ```bash
    # 1. Configure build directory
    cmake -G "Ninja" -S . -B build

    # 2. Compile Release binary
    cmake --build build --config Release

    # 3. Launch MemoraDB
    # On Windows:
    .\build\memora.exe
    # On Linux / macOS:
    ./build/memora
    ```

=== "Without Ninja (MinGW / Make)"

    ```bash
    # On Windows (MinGW):
    cmake -G "MinGW Makefiles" -S . -B build
    cmake --build build --config Release
    .\build\memora.exe

    # On Linux / macOS (Unix Makefiles):
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build
    ./build/memora
    ```

---

### 4. Running the Developer Test Suite (Optional)

MemoraDB includes an automated unit and integration test suite powered by GoogleTest and CTest. Tests are disabled by default during normal builds and can be enabled with `-DMEMORA_BUILD_TESTS=ON`:

```bash
# Configure and build tests
cmake -G "Ninja" -S . -B build-tests -DMEMORA_BUILD_TESTS=ON
cmake --build build-tests

# Run all tests
ctest --test-dir build-tests --output-on-failure
```

For detailed test suite documentation, test filters, and architectural testing details, see the [Testing Guide](../development/testing.md).

---

## Why 64-Bit Architecture is Required

MemoraDB requires a 64-bit operating system (`x86_64` / `AMD64` or `ARM64`):

1. **Precompiled ONNX Runtime Engine**: Microsoft distributes modern pre-built ONNX Runtime C++ release binaries exclusively for 64-bit platforms (`win-x64`, `linux-x64`, `osx-universal2`). There are no 32-bit builds available.
2. **Dense Vector Memory Space**: In-process inference and vector indexes process 384-dimensional floating-point tensors (`float[384] = 1,536 bytes` per semantic row). Managing large vector candidate sets in memory requires a 64-bit virtual address space.
3. **64-Bit Disk Offsets & Timestamps**: The binary storage engine uses 64-bit unsigned integers (`uint64_t`) for file byte seek offsets and millisecond epoch timestamps, ensuring continuous durability without 32-bit integer overflow limits.

---

## Verifying the Installation

Launch the executable from your terminal:

```bash
# Windows
.\build\memora.exe

# Linux / macOS
./build/memora
```

Verify the status indicators printed below the startup banner:

```text
  ●  temporal engine ready              ●  append-only storage ready
  ●  semantic vector search ready        ●  embedding model ready
```

* All green dots (`●`) indicate that storage, temporal indexing, and semantic embedding inference are fully initialized.
* Type `.about` to verify database version information.
* Type `.exit` or `.quit` to leave the REPL.
