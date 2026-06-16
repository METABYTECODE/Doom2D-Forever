@echo off
setlocal
cd /d "%~dp0"

set "ORGANIZE=build\tools\d2df_organize\Release\d2df-organize.exe"
set "MANIFEST=assets\staging\manifest.json"

if not exist "%ORGANIZE%" (
    echo [ERROR] d2df-organize not found. Run build.bat first.
    exit /b 1
)

if not exist "%MANIFEST%" (
    echo [ERROR] Staging manifest not found. Run extract_assets.bat first.
    echo   Expected: %MANIFEST%
    exit /b 1
)

echo === Organize assets ===
echo   Staging: assets\staging\
echo   Output:  assets\content\
echo.

"%ORGANIZE%" --staging assets\staging --output assets\content --overrides assets\organize_overrides.json
if errorlevel 1 goto :failed

echo.
echo === Done ===
echo   Manifest: assets\content\manifest.json
echo   Aliases:  assets\content\aliases.json
exit /b 0

:failed
echo.
echo [ERROR] Organize failed.
exit /b 1
