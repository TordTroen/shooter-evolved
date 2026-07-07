@echo off
setlocal

cd /d "%~dp0"

set VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat

if not exist "%VCVARS%" (
    echo [test] ERROR: Could not find vcvars64.bat
    exit /b 1
)

call "%VCVARS%" > nul 2>&1

cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON || exit /b 1
cmake --build cmake-build-debug --target fps_demo_tests || exit /b 1
ctest --test-dir cmake-build-debug --output-on-failure
endlocal