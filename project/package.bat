@echo off
python "%~dp0tools\build.py" package --platform windows %*
exit /b %errorlevel%
