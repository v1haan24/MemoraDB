# Dependencies

This page documents the external libraries, pre-trained neural models, and system dependencies utilized by MemoraDB.

---

## Dependency Overview

| Dependency | Version | Type | Source / Acquisition |
|---|---|---|---|
| **C++ Standard Library** | C++17 | Build & Runtime | Provided by host compiler (MSVC, GCC, Clang). |
| **Microsoft ONNX Runtime** | 1.22.0 | Build & Runtime | Automatically downloaded via CMake `FetchContent`. |
| **`all-MiniLM-L6-v2` Model** | Main | Runtime Data | Automatically downloaded from Hugging Face via CMake. |
| **CMake** | 3.20+ | Build Tool | Host installation required. |
| **Ninja / Make** | Latest | Build Tool | Optional but recommended build generator. |

---

## 1. Microsoft ONNX Runtime (v1.22.0)

ONNX Runtime is Microsoft's high-performance inference engine for Open Neural Network Exchange (ONNX) models.

* **Role**: Executes the `all-MiniLM-L6-v2` transformer model directly inside the `memora` process.
* **Header Files**: `onnxruntime_cxx_api.h`
* **Acquisition**: Managed automatically by `CMakeLists.txt` via `FetchContent`:
  * Windows x64: `https://github.com/microsoft/onnxruntime/releases/download/v1.22.0/onnxruntime-win-x64-1.22.0.zip`
  * Linux x64: `https://github.com/microsoft/onnxruntime/releases/download/v1.22.0/onnxruntime-linux-x64-1.22.0.tgz`
  * macOS universal: `https://github.com/microsoft/onnxruntime/releases/download/v1.22.0/onnxruntime-osx-universal2-1.22.0.tgz`
* **Runtime Deployment**:
  * On Windows, CMake copies `onnxruntime.dll` and `onnxruntime_providers_shared.dll` into the output directory next to `memora.exe`.
  * On Linux/macOS, `libonnxruntime.so` or `libonnxruntime.dylib` is copied to the binary folder.

---

## 2. Sentence Transformer Model: `all-MiniLM-L6-v2`

The sentence embedding model maps arbitrary natural language text to dense 384-dimensional floating-point vectors.

* **Provider**: Hugging Face (`sentence-transformers/all-MiniLM-L6-v2`).
* **Assets**:
  1. `model.onnx` (~86 MB): Quantized/optimized ONNX computation graph.
  2. `vocab.txt` (~232 KB): WordPiece token vocabulary.
* **Storage Location**: Located adjacent to the executable at `models/all-MiniLM-L6-v2/`.
* **Acquisition**: Downloaded automatically during the CMake configure phase using `file(DOWNLOAD ... TLS_VERIFY ON)`.

---

## 3. C++ Standard Library Features

MemoraDB strictly relies on ISO C++17 language and library features, avoiding third-party container or utility libraries:

* `std::variant`: Powers the type-safe AST representation (`Statement`).
* `std::optional`: Represents optional clauses (`WHERE`, `ORDER BY`, `LIMIT`).
* `std::filesystem`: Cross-platform directory creation, file size verification, atomic renaming, and file truncation.
* `std::chrono`: High-resolution wall-clock timestamp acquisition for record headers.
* `std::priority_queue`: Min-heap used to rank top-k semantic search results.
