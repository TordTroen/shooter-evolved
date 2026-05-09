@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cd /D "C:\projects\c++\shooter\project\cmake-build-debug"
cmake --build . 2>&1
echo EXIT_CODE=%ERRORLEVEL%
