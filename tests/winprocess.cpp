// winprocess.cpp
//
// Implementation of winprocess.h. This is the ONLY file in the test that
// includes <windows.h> -- see winprocess.h for why that matters (its
// typedefs for INT/FLOAT/BOOL collide with this project's DataType enum).
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "winprocess.h"

#include <iostream>

namespace fs = std::filesystem;

struct ChildProcessHandle {
    HANDLE hProcess = nullptr;
    DWORD pid = 0;
};

static std::string buildCommandLine(const std::vector<std::string>& argv) {
    std::string cmd;
    for (size_t i = 0; i < argv.size(); ++i) {
        if (i) cmd += ' ';
        cmd += '"';
        cmd += argv[i];
        cmd += '"';
    }
    return cmd;
}

ChildProcessHandle* spawnProcessWin(const std::vector<std::string>& argv,
                                     const fs::path& logPath) {
    fs::path parent = logPath.parent_path();
    fs::create_directories(parent.empty() ? fs::path(".") : parent);

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hLog = CreateFileA(logPath.string().c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                               &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hLog == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create log file '" << logPath.string()
                   << "' (error " << GetLastError() << ")\n";
        return nullptr;
    }

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdOutput = hLog;
    si.hStdError = hLog;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi{};

    std::string cmdLine = buildCommandLine(argv);
    std::vector<char> cmdLineBuf(cmdLine.begin(), cmdLine.end());
    cmdLineBuf.push_back('\0');

    BOOL ok = CreateProcessA(
        nullptr,                    // use command line's first token as the module
        cmdLineBuf.data(),          // mutable command line buffer (Win32 requirement)
        nullptr, nullptr,
        TRUE,                       // inherit handles (so hLog reaches the child)
        CREATE_NEW_PROCESS_GROUP,   // isolate from our own console signals
        nullptr,                    // inherit environment
        nullptr,                    // inherit current directory
        &si, &pi);

    CloseHandle(hLog); // child holds its own inherited copy

    if (!ok) {
        std::cerr << "CreateProcess failed for [" << cmdLine << "] (error "
                   << GetLastError() << ")\n";
        return nullptr;
    }

    CloseHandle(pi.hThread);

    auto* handle = new ChildProcessHandle();
    handle->hProcess = pi.hProcess;
    handle->pid = pi.dwProcessId;
    return handle;
}

void terminateProcessWin(ChildProcessHandle* handle) {
    if (!handle) return;
    if (handle->hProcess) {
        TerminateProcess(handle->hProcess, 0);
        WaitForSingleObject(handle->hProcess, 5000);
        CloseHandle(handle->hProcess);
        handle->hProcess = nullptr;
    }
    delete handle;
}
