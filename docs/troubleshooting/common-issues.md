# Troubleshooting & Common Issues

This guide addresses common build, configuration, and runtime issues you may encounter when working with MemoraDB and provides verified solutions.

---

## Build & CMake Issues

### 1. CMake Defaults to NMake or Fails to Find Generator

> [!WARNING]
> **Error**: `Running 'nmake' '-?' failed with: no such file or directory` or `CMAKE_CXX_COMPILER not set`.

**Cause**: On Windows, CMake defaults to Visual Studio or NMake if no generator is specified, but NMake is not in your PATH.

**Resolution**: Explicitly specify your installed generator:
* If you have Ninja installed:
  ```powershell
  cmake -G "Ninja" -S . -B build
  ```
* If you are using MinGW (`gcc` / `mingw32-make`):
  ```powershell
  cmake -G "MinGW Makefiles" -S . -B build
  ```

---

### 2. Missing 64-Bit Architecture on Windows

> [!WARNING]
> **Error**: `Automatic ONNX Runtime download currently supports Windows x64 only`.

**Cause**: CMake detected a 32-bit compiler environment or non-x64 CPU. Modern ONNX Runtime precompiled binaries require a 64-bit platform (`x86_64` / `AMD64`).

**Resolution**: Ensure you are using a 64-bit compiler:
* For Visual Studio: open **x64 Native Tools Command Prompt for VS 2022**.
* For MinGW: use the UCRT64 or MINGW64 environment (`C:\msys64\ucrt64\bin`).
* To provide your own pre-built ONNX Runtime build, set:
  ```powershell
  cmake -S . -B build -DONNXRUNTIME_ROOT="C:/path/to/onnxruntime"
  ```

---

### 3. Model Asset Download Timeout (Hugging Face)

> [!WARNING]
> **Error**: `Failed to download model asset onnx/model.onnx` during CMake configure.

**Cause**: Network restrictions, corporate firewalls, or proxy settings preventing CMake from downloading the transformer model from Hugging Face.

**Resolution**:
1. Manually download `model.onnx` and `vocab.txt` from [Hugging Face all-MiniLM-L6-v2](https://huggingface.co/sentence-transformers/all-MiniLM-L6-v2/tree/main).
2. Create the destination directory and place both files inside:
   ```text
   build/models/all-MiniLM-L6-v2/model.onnx
   build/models/all-MiniLM-L6-v2/vocab.txt
   ```
3. Re-run CMake configure. It will detect the existing files and skip the download.

---

## Runtime & Shell Issues

### 1. "embedding model ready (off)" on Startup

> [!WARNING]
> **Status Display**: `● semantic vector search ready (off) ● embedding model ready (off)`.

**Cause**: The executable could not locate `models/all-MiniLM-L6-v2/model.onnx` or `vocab.txt`. MemoraDB searches for this folder relative to the binary's actual location on disk.

**Resolution**:
* Ensure the `models/all-MiniLM-L6-v2/` folder is placed in the **exact same directory** as `memora.exe` (or `memora`).
* If running from PowerShell inside `build/`:
  ```powershell
  Test-Path .\build\models\all-MiniLM-L6-v2\model.onnx
  ```

---

### 2. Missing DLL Error on Windows (`onnxruntime.dll`)

> [!WARNING]
> **Error**: `onnxruntime.dll was not found` or system error code `0xc0000135`.

**Cause**: The ONNX Runtime shared libraries are missing from the folder containing `memora.exe`.

**Resolution**:
Copy `onnxruntime.dll` and `onnxruntime_providers_shared.dll` into the directory containing `memora.exe`:
```powershell
# If using CMake FetchContent:
Copy-Item .\build\_deps\onnxruntime-src\lib\*.dll .\build\
```

---

## SQL & Query Issues

### 1. "Cannot UPDATE the primary key column"

> [!WARNING]
> **Error**: `Cannot UPDATE the primary key column '<col>' (delete and re-insert instead)`.

**Cause**: MemoraDB uses an immutable append-only storage model. The primary key anchors the version lineage of a record. Overwriting a primary key in place would break time-travel consistency across versions.

**Resolution**: Delete the record and insert a new record with the updated primary key:
```sql
DELETE FROM users WHERE id = 1;
INSERT INTO users VALUES (2, 'Alice');
```

---

### 2. "Table name exceeds 29 characters" or "Column name exceeds 29 characters"

> [!WARNING]
> **Error**: `Table name exceeds 29 characters` or `Column name exceeds 29 characters`.

**Cause**: Fixed metadata headers define `tns = 30` (Table Name Size) and `cns = 30` (Column Name Size), allocating 29 characters for the name plus 1 byte for the null terminator (`\0`).

**Resolution**: Use identifiers up to 29 characters in length (e.g., `user_profiles` instead of `user_application_account_profiles_table`).

---

### 3. Parse Error on Compound `WHERE` Clause (`AND` / `OR`)

> [!WARNING]
> **Error**: Syntax error when executing `WHERE a = 1 AND b = 2`.

**Cause**: MemoraDB's SQL dialect currently supports **exactly one** comparison condition per `WHERE` clause.

**Resolution**: Filter by the most restrictive condition first, or project and inspect matching records:
```sql
-- Supported:
SELECT * FROM orders WHERE status = 'pending';

-- Unsupported:
SELECT * FROM orders WHERE status = 'pending' AND total > 100;
```

---

### 4. `SIMILAR TO` Fails on Standard Column

> [!WARNING]
> **Error**: `Column '<col>' is not declared SEMANTIC, so SIMILAR TO can't be used on it`.

**Cause**: Vector similarity search requires pre-computed dense embeddings. Only `STRING` columns declared with the `SEMANTIC` modifier during `CREATE TABLE` have vector index tables generated.

**Resolution**: Recreate the table with `SEMANTIC` on the text column:
```sql
CREATE TABLE articles (
    id INT PRIMARY KEY,
    body STRING(300) SEMANTIC
);
```

---

## Crash Recovery & Storage

### Corrupted Record Warning on Startup

> [!NOTE]
> **Diagnostic Message**: `Corrupted record encountered during recovery.`

**Explanation**: A previous DBMS process was terminated abruptly (e.g., power loss, killed process) while writing a binary record to `data.db`.

**Engine Behavior**: MemoraDB automatically scans the binary log on startup. When an incomplete or corrupt record header is detected, the engine automatically truncates the file back to the last known valid record boundary using `std::filesystem::resize_file`. No committed records are lost, and the database remains in a consistent state.
