@echo off
setlocal
cd /d "%~dp0"

set "EXE=build\src\d2df_client\Release\d2df.exe"

if not exist "%EXE%" (
    echo [ERROR] Client not found. Run build.bat first.
    echo   Expected: %EXE%
    pause
    exit /b 1
)

if not exist "assets\content\manifest.json" (
    echo [ERROR] Content not found. Ensure assets\content\manifest.json exists.
    pause
    exit /b 1
)

"%EXE%" %*
set "RC=%ERRORLEVEL%"
if %RC% neq 0 pause
exit /b %RC%
