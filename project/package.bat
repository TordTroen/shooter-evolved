@echo off
setlocal enabledelayedexpansion

rem Usage: package.bat [zip-name]
rem   zip-name  Optional. If given, also zips the release folder's contents
rem             into "%OUT_DIR%\<zip-name>.zip" (".zip" appended if missing).

cd /d "%~dp0"

set BUILD_DIR=cmake-build-debug
set OUT_DIR=release
set ZIP_NAME=%~1

call "%~dp0build.bat" release
if errorlevel 1 (
    echo [package] Build failed.
    exit /b 1
)

echo [package] Assembling %OUT_DIR%...
if exist "%OUT_DIR%" rmdir /s /q "%OUT_DIR%"
mkdir "%OUT_DIR%"

copy /y "%BUILD_DIR%\fps_demo.exe" "%OUT_DIR%\" >nul
if errorlevel 1 (
    echo [package] ERROR: fps_demo.exe not found in %BUILD_DIR%.
    exit /b 1
)

for %%D in (SDL3.dll GameNetworkingSockets.dll abseil_dll.dll libcrypto-3-x64.dll libprotobuf.dll) do (
    copy /y "%BUILD_DIR%\%%D" "%OUT_DIR%\" >nul
)

xcopy /e /i /y /q "shaders" "%OUT_DIR%\shaders" >nul
xcopy /e /i /y /q "assets" "%OUT_DIR%\assets" >nul

copy /y "allow_firewall.bat" "%OUT_DIR%\" >nul

if not "%ZIP_NAME%"=="" (
    if /I not "!ZIP_NAME:~-4!"==".zip" set ZIP_NAME=!ZIP_NAME!.zip

    if exist "%OUT_DIR%\!ZIP_NAME!" del /f /q "%OUT_DIR%\!ZIP_NAME!"

    echo [package] Zipping to %OUT_DIR%\!ZIP_NAME!...
    powershell -NoProfile -Command "Compress-Archive -Path '%OUT_DIR%\*' -DestinationPath '%OUT_DIR%\!ZIP_NAME!' -Force"
    if errorlevel 1 (
        echo [package] ERROR: Failed to create zip.
        exit /b 1
    )
)

echo [package] Done. Release folder: %OUT_DIR%
endlocal
