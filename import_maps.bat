@echo off
setlocal
cd /d "%~dp0"

set "IMPORT=build\tools\d2df_map_import\Release\d2df-map-import.exe"
set "VALIDATE=build\tools\d2df_content_validate\Release\d2df-content-validate.exe"
set "SUMMARY=assets\content\maps\import_summary.txt"

if not exist "%IMPORT%" (
    echo [ERROR] d2df-map-import not found. Run build.bat first.
    exit /b 1
)

if not exist "assets\content\manifest.json" (
    echo [ERROR] Content not found. Run organize_assets.bat first.
    exit /b 1
)

echo === Import maps (mapbin -^> JSON) ===
echo   Content: assets\content\
echo.

"%IMPORT%" --content assets\content
set "RC=%ERRORLEVEL%"

if exist "%SUMMARY%" type "%SUMMARY%"

if %RC% neq 0 goto :failed

if exist "%VALIDATE%" (
    echo.
    echo === Validate content ===
    "%VALIDATE%" --content assets\content
    set "RC=%ERRORLEVEL%"
)

if %RC% neq 0 goto :failed
exit /b 0

:failed
echo.
echo [ERROR] Map import/validate failed.
exit /b 1
