@echo off
setlocal
cd /d "%~dp0"

echo ========================================================
echo Building NavSys C Navigation Engine
echo ========================================================

REM Automatically include MSYS2 / MinGW in PATH if not already set
if exist "C:\msys64\ucrt64\bin" set "PATH=C:\msys64\ucrt64\bin;%PATH%"
if exist "C:\msys64\mingw64\bin" set "PATH=C:\msys64\mingw64\bin;%PATH%"

REM Check for GCC compiler
where gcc >nul 2>&1
if errorlevel 1 (
    echo [ERROR] GCC compiler not found in PATH or C:\msys64\ucrt64\bin!
    echo Please ensure GCC is installed.
    echo ========================================================
    pause
    exit /b 1
)

for /f "delims=" %%i in ('where gcc') do set "FOUND_GCC=%%i"
echo Using C Compiler: %FOUND_GCC%

REM Stop running server instance if any, to avoid file lock on navsys.exe
taskkill /f /im navsys.exe >nul 2>&1

echo Compiling navsys.c ...
gcc -O2 navsys.c -o navsys.exe -lws2_32
if errorlevel 1 (
    echo.
    echo [ERROR] Native C compilation failed!
    echo ========================================================
    pause
    exit /b 1
)

echo.
echo [SUCCESS] Successfully compiled native C binary: navsys.exe
echo ========================================================
echo Build complete. You can now double-click run.bat!
echo ========================================================
pause
