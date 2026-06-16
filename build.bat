@echo off
setlocal
cd /d "%~dp0"

if not defined VCPKG_ROOT set "VCPKG_ROOT=C:\vcpkg"

if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    echo [ERROR] vcpkg not found at: %VCPKG_ROOT%
    echo Set VCPKG_ROOT or install vcpkg to C:\vcpkg
    exit /b 1
)

echo === Doom2D Forever: configure ===
cmake -B build -S . ^
  -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
  -DVCPKG_FEATURE_FLAGS=manifests
if errorlevel 1 goto :failed

echo.
echo === Doom2D Forever: build Release ===
cmake --build build --config Release
if errorlevel 1 goto :failed

echo.
echo === Done ===
echo   Client:  build\src\d2df_client\Release\d2df.exe
echo   Extract: build\tools\d2df_extract\Release\d2df-extract.exe
echo   Organize: build\tools\d2df_organize\Release\d2df-organize.exe
exit /b 0

:failed
echo.
echo [ERROR] Build failed.
exit /b 1
