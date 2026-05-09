@echo off
setlocal

cd /d "%~dp0"

set BUILD_DIR=cmake-build-debug
set BUILD_TYPE=Debug
set VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat

if /i "%1"=="release" set BUILD_TYPE=Release

if not exist "%VCVARS%" (
    echo [build] ERROR: Could not find vcvars64.bat
    exit /b 1
)

call "%VCVARS%" > nul 2>&1

if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo [build] Configuring CMake %BUILD_TYPE%...
    cmake -S . -B %BUILD_DIR% -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
    if errorlevel 1 (
        echo [build] CMake configure failed.
        exit /b 1
    )
)

echo [build] Building...
cmake --build %BUILD_DIR% --config %BUILD_TYPE%
if errorlevel 1 (
    echo [build] Build failed.
    exit /b 1
)

echo [build] Done. Executable: %BUILD_DIR%\fps_demo.exe
endlocal
