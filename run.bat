@echo off
setlocal
cd /d "%~dp0"

if not exist "navsys.exe" (
    echo navsys.exe not found. Building first...
    call build.bat
)

echo Starting NavSys C Navigation Server on http://localhost:8000 ...
start "" http://localhost:8000
navsys.exe --serve 8000
pause
