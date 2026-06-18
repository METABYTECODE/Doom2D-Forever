@echo off
setlocal
cd /d "%~dp0"

set "EXE=build\src\rivet_playground\Release\rivet_engine.exe"
if not exist "%EXE%" set "EXE=build\src\rivet_playground\Debug\rivet_engine.exe"

if not exist "%EXE%" (
    echo [ERROR] Rivet Engine playground not found. Run build.bat first.
    echo   Expected: build\src\rivet_playground\Debug\rivet_engine.exe
    pause
    exit /b 1
)

if "%~1"=="" (
    "%EXE%" level
) else (
    "%EXE%" %*
)

set "RC=%ERRORLEVEL%"
if %RC% neq 0 pause
exit /b %RC%
