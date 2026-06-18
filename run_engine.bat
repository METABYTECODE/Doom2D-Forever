@echo off
setlocal
cd /d "%~dp0"

set "EXE=build\src\d2df_engine_playground\Release\d2df_engine.exe"
if not exist "%EXE%" set "EXE=build\src\d2df_engine_playground\Debug\d2df_engine.exe"

if not exist "%EXE%" (
    echo [ERROR] Engine playground not found. Run build.bat first.
    echo   Expected: build\src\d2df_engine_playground\Release\d2df_engine.exe
    pause
    exit /b 1
)

if not exist "assets\content\manifest.json" (
    echo [ERROR] Content not found. Ensure assets\content\manifest.json exists.
    pause
    exit /b 1
)

if "%~1"=="" (
    "%EXE%" --content assets\content --map maps\doom2d\map01.json
) else (
    "%EXE%" %*
)

set "RC=%ERRORLEVEL%"
if %RC% neq 0 pause
exit /b %RC%
