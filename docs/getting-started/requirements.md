# System Requirements

This page details the software, hardware, and runtime requirements for compiling and running MemoraDB.

---

## Hardware Requirements

| Component | Minimum | Recommended |
|---|---|---|
| **Architecture** | 64-bit x86 (`x86_64` / `AMD64`) or Apple Silicon (`arm64` via universal binary) | 64-bit x86_64 with AVX2 instruction support |
| **CPU** | 2 cores | 4+ cores (accelerates ONNX embedding inference) |
| **RAM** | 2 GB | 4 GB+ (to accommodate the MiniLM model in memory) |
| **Disk Space** | 500 MB free space | 2 GB+ (for build cache, ONNX runtime, and model assets) |

---

## Operating System Support

MemoraDB's CMake configuration includes automatic platform detection and FetchContent hooks for the following 64-bit operating systems:

* **Windows**: Windows 10, 11, or Windows Server (x86_64).
* **Linux**: Modern Linux distributions (Ubuntu 20.04+, Debian 11+, Fedora 36+, Arch Linux) with `glibc` 2.31+ on `x86_64`.
* **macOS**: macOS 11 (Big Sur) or newer on Intel or Apple Silicon (`universal2` ONNX archive supported).

> [!NOTE]
> 32-bit platforms (e.g., x86 32-bit, ARM32) are explicitly **not supported** by the automatic ONNX Runtime download script in `CMakeLists.txt`.

---

## Build Prerequisites

To compile MemoraDB from source, ensure the following tools are installed and present in your system `PATH`:

### 1. C++ Compiler (C++17 Support Required)

MemoraDB strictly relies on ISO C++17 language features (`std::variant`, `std::optional`, `std::filesystem`, structured bindings, and inline variables).

* **Windows**: Visual Studio 2019 / 2022 (MSVC v142/v143) with the C++ Desktop Development workload, or MinGW-w64 (GCC 9+).
* **Linux**: GCC 9.0+ or Clang 10.0+.
* **macOS**: Xcode Command Line Tools 12.0+ (Apple Clang).

### 2. CMake

* **Version**: CMake **3.20** or higher is required (`cmake_minimum_required(VERSION 3.20)`).
* Verification:
  ```bash
  cmake --version
  ```

### 3. Build Generator

* **Ninja** (Recommended for fast incremental builds on all platforms):
  ```bash
  ninja --version
  ```
* Alternatively, Visual Studio Solution generators on Windows or GNU Make on Linux.

### 4. Network Access (Build-Time Only)

During the initial CMake configure phase, CMake will automatically download:
1. **Microsoft ONNX Runtime 1.22.0**:
   * Windows: `onnxruntime-win-x64-1.22.0.zip` (~45 MB)
   * Linux: `onnxruntime-linux-x64-1.22.0.tgz` (~55 MB)
   * macOS: `onnxruntime-osx-universal2-1.22.0.tgz` (~50 MB)
2. **Hugging Face Model Assets** (`sentence-transformers/all-MiniLM-L6-v2`):
   * `model.onnx` (~86 MB)
   * `vocab.txt` (~232 KB)

If you are working in an air-gapped or offline environment, you must manually populate `ONNXRUNTIME_ROOT` and the model directory prior to configuring CMake.

---

## Runtime Requirements

When executing `memora` (or `memora.exe` on Windows):

1. **Shared Libraries**:
   * On Windows: `onnxruntime.dll` and `onnxruntime_providers_shared.dll` must be in the same folder as `memora.exe` (the build system automatically copies them post-build).
   * On Linux/macOS: The ONNX dynamic libraries must be in the executable directory or discoverable via `LD_LIBRARY_PATH` / `DYLD_LIBRARY_PATH`.
2. **Model Assets**:
   * The directory `models/all-MiniLM-L6-v2/` containing `model.onnx` and `vocab.txt` must reside adjacent to the executable. The executable dynamically locates this directory relative to its own binary path using platform-specific APIs (`_get_pgmptr` on Windows, `/proc/self/exe` on Linux, and `_NSGetExecutablePath` on macOS).
3. **Data Directory**:
   * MemoraDB creates and manages a local `data/` directory relative to the current working directory from which you launch the REPL. Ensure write permissions exist in that directory.
