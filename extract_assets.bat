@echo off
setlocal
cd /d "%~dp0"

set "EXTRACT=build\tools\d2df_extract\Release\d2df-extract.exe"
set "LOG=assets\staging\extract.log"
set "SUMMARY=assets\staging\extract_summary.txt"

if not exist "%EXTRACT%" (
    echo [ERROR] d2df-extract not found. Run build.bat first.
    exit /b 1
)

if not exist "assets" (
    echo [ERROR] Folder not found: assets\
    exit /b 1
)

where ffmpeg >nul 2>&1
if errorlevel 1 (
    echo [WARN] ffmpeg not in PATH — raw formats only, no OGG/PNG conversion.
    set "EXTRA_FLAGS=--no-convert-audio --no-convert-images"
) else (
    set "EXTRA_FLAGS="
)

if not exist "assets\staging" mkdir "assets\staging"

echo === Extract assets ===
echo   Input:  assets\
echo   Output: assets\staging\
echo   Log:    %LOG%
echo.

"%EXTRACT%" --input assets --output assets\staging %EXTRA_FLAGS% > "%LOG%" 2>&1
set "RC=%ERRORLEVEL%"

if exist "%SUMMARY%" (
    type "%SUMMARY%"
) else (
    echo [WARN] Summary not found. Last lines of log:
    powershell -NoProfile -Command "Get-Content -Path '%LOG%' -Tail 30"
)

if %RC% neq 0 goto :failed

echo.
echo Full log: %LOG%
echo Report:   assets\staging\extract_report.json
exit /b 0

:failed
echo.
echo [ERROR] Extract failed. See %LOG%
exit /b 1
