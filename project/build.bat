@echo off
python "%~dp0tools\build.py" build --platform windows %*
exit /b %errorlevel%
