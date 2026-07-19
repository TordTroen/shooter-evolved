@echo off
python "%~dp0tools\build.py" test --platform windows %*
exit /b %errorlevel%
