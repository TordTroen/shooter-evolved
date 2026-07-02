@echo off
rem Adds a Windows Firewall rule allowing inbound UDP traffic for fps_demo.exe,
rem so other PCs on the same LAN can connect when this machine hosts a game.
rem Usage: allow_firewall.bat [remove]
rem   remove  Optional. Removes the rule instead of adding it.
rem Must be run as Administrator.

set RULE_NAME=FpsEvolved

net session >nul 2>&1
if errorlevel 1 (
    echo [firewall] This script must be run as Administrator.
    echo [firewall] Right-click this file and choose "Run as administrator".
    pause
    exit /b 1
)

if /I "%~1"=="remove" (
    netsh advfirewall firewall delete rule name="%RULE_NAME%"
    if errorlevel 1 (
        echo [firewall] ERROR: Failed to remove firewall rule.
        pause
        exit /b 1
    )
    echo [firewall] Rule removed: %RULE_NAME%
    pause
    exit /b 0
)

set EXE_PATH=%~dp0fps_demo.exe

if not exist "%EXE_PATH%" (
    echo [firewall] ERROR: fps_demo.exe not found next to this script.
    pause
    exit /b 1
)

netsh advfirewall firewall delete rule name="%RULE_NAME%" >nul 2>&1

netsh advfirewall firewall add rule name="%RULE_NAME%" dir=in action=allow program="%EXE_PATH%" protocol=UDP enable=yes profile=private,public
if errorlevel 1 (
    echo [firewall] ERROR: Failed to add firewall rule.
    pause
    exit /b 1
)

echo [firewall] Rule added: inbound UDP allowed for %EXE_PATH%
pause
