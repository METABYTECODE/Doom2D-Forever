# Doom2D Forever — Build

Modern C++ rewrite of [Doom2D Forever](https://doom2d.org).

> **Status:** playable prototype with engine + game bridge. See [docs/07-PROJECT-STATUS.md](docs/07-PROJECT-STATUS.md).

## Quick start (Windows)

```bat
build.bat
run_game.bat
run_engine.bat
```

Requires vcpkg (`VCPKG_ROOT` or `C:\vcpkg`).

Runtime assets live in `assets/content/` (maps, textures, sounds).

## Build (manual)

### Requirements

- CMake 3.25+
- C++20 compiler (MSVC 2022, GCC 12+, Clang 15+)
- [vcpkg](https://vcpkg.io/) manifest mode (`vcpkg.json`)

### Windows

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
$env:VCPKG_ROOT = "C:\vcpkg"

cmake -B build -S . `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_FEATURE_FLAGS=manifests

cmake --build build --config Release
ctest --test-dir build --output-on-failure -C Release
```

### Run

```powershell
.\build\src\d2df_client\Release\d2df.exe
.\build\src\d2df_engine_playground\Release\d2df_engine.exe --content assets\content --map maps\doom2d\map01.json
.\build\src\d2df_server\Release\d2df_server.exe
```

### Asset import tools (optional)

Used to build `assets/content/` from legacy WAD archives (not shipped in repo).

| Tool | Purpose |
|------|---------|
| `d2df-wad-ls` | List DFWAD/ZIP entries |
| `d2df-extract` | Legacy archives → staging folder |
| `d2df-organize` | Staging → `assets/content/` with logical dirs |
| `d2df-tilemap-convert` | Legacy JSON → `d2df-tilemap` export |

```powershell
.\build\tools\d2df_wad_ls\Release\d2df-wad-ls.exe path\to\game.WAD

.\build\tools\d2df_extract\Release\d2df-extract.exe --input path\to\wads --output assets\staging
.\build\tools\d2df_organize\Release\d2df-organize.exe --staging assets\staging --output assets\content
```

Requires `ffmpeg` in PATH for audio/image conversion.

### Assets layout

```
assets/
├── content/              # runtime target (maps, manifest, textures, audio)
└── organize_overrides.json
```

See [docs/](docs/) for architecture and roadmap.
