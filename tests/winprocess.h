// winprocess.h
//
// A tiny, windows.h-free interface for spawning/watching/killing child
// processes. This exists ONLY to keep <windows.h> out of any translation
// unit that also needs common/metadata.h -- windows.h's minwindef.h
// typedefs INT, FLOAT, and BOOL as Win32 types, which collide directly
// with this project's `enum DataType {INT, FLOAT, STRING, BOOL}`. Isolating
// the Win32 calls here (implemented in winprocess.cpp, which DOES include
// windows.h) means the rest of the test can #include the project's real
// headers without any renaming or reordering tricks.
#pragma once

#include <filesystem>
#include <string>
#include <vector>

// Opaque handle -- the actual Win32 HANDLE/PID live inside winprocess.cpp.
struct ChildProcessHandle;

// Spawns `argv[0] argv[1] ...` as a new process with stdout/stderr
// redirected to logPath. Returns nullptr on failure.
ChildProcessHandle* spawnProcessWin(const std::vector<std::string>& argv,
                                     const std::filesystem::path& logPath);

// Forcefully terminates the process (if still running), waits briefly for
// it to exit, and releases the handle. Safe to call on a nullptr handle.
void terminateProcessWin(ChildProcessHandle* handle);
