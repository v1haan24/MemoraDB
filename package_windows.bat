@echo off
setlocal enabledelayedexpansion

echo ============================================================
echo MemoraDB Windows Packaging Script
echo ============================================================

REM Check if build\release exists, if not build it
if not exist "build\release\memora.exe" (
    echo [1/3] Configuring Release build with CMake...
    cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
    if errorlevel 1 (
        echo CMake configuration failed.
        exit /b 1
    )

    echo [2/3] Building MemoraDB and staging runtime DLLs...
    cmake --build build --config Release
    if errorlevel 1 (
        echo Build failed.
        exit /b 1
    )
) else (
    echo Found existing build\release\memora.exe
)

echo [3/3] Compiling Inno Setup Installer...
where iscc >nul 2>nul
if errorlevel 1 (
    echo Error: Inno Setup compiler (iscc) was not found in PATH.
    echo Please install Inno Setup 6 (or run 'choco install innosetup')
    echo and ensure iscc.exe is added to your PATH.
    exit /b 1
)

iscc installer.iss
if errorlevel 1 (
    echo Inno Setup compilation failed.
    exit /b 1
)

echo.
echo ============================================================
echo Windows Setup EXE Generated Successfully!
echo Output: MemoraDB-Setup-v1.0.0.exe
echo ============================================================
