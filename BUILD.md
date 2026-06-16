# Doom2D Forever — C++ Rewrite



Modern C++ rewrite of [Doom2D Forever](https://doom2d.org). Pascal sources in `src/` are reference; new code lives under `src/d2df*`.



> **Статус:** Phase 0 + 1a + 1b done → next: Phase 2b. See [docs/07-PROJECT-STATUS.md](docs/07-PROJECT-STATUS.md).



## Quick start (Windows batch files)



```bat

build.bat              REM configure + Release build

extract_assets.bat     REM assets/ → assets/staging/

organize_assets.bat    REM assets/staging/ → assets/content/

```



Requires vcpkg (`VCPKG_ROOT` or `C:\vcpkg`) and ffmpeg in PATH for conversion.



## Build (manual)



### Requirements



- CMake 3.25+

- C++20 compiler (MSVC 2022, GCC 12+, Clang 15+)

- [vcpkg](https://vcpkg.io/) with manifest mode (dependencies in `vcpkg.json`)



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

.\build\src\d2df_client\Release\d2df.exe      # SDL window, ESC to quit

.\build\src\d2df_server\Release\d2df_server.exe

```



### Asset tools



| Tool | Purpose |

|------|---------|

| `d2df-wad-ls` | List DFWAD/ZIP entries |

| `d2df-extract` | Legacy archives → `assets/staging/` |

| `d2df-organize` | Staging → `assets/content/` with logical dirs |



```powershell

# List archive

.\build\tools\d2df_wad_ls\Release\d2df-wad-ls.exe assets\data\game.WAD



# Full pipeline

.\build\tools\d2df_extract\Release\d2df-extract.exe --input assets --output assets\staging

.\build\tools\d2df_organize\Release\d2df-organize.exe --staging assets\staging --output assets\content



# Fast extract (no ffmpeg)

.\build\tools\d2df_extract\Release\d2df-extract.exe --input assets --output assets\staging --no-convert-audio --no-convert-images

```



Requires `ffmpeg` in PATH for audio/image conversion (WAV/MOD/MP3/TGA/JPEG/MIDI → OGG/PNG).



Nested anim textures (e.g. `COL5A0`) are unwrapped automatically — see [docs/05-FORMAT-SPEC.md](docs/05-FORMAT-SPEC.md) §1.6.



### Assets layout



```

assets/

├── data/      # game.WAD, standart.WAD, models/, ...

├── maps/      # map packs (.wad, .dfz)

├── wads/      # duplicate WADs (deduped on organize)

├── staging/   # d2df-extract output (gitignored)

└── content/   # d2df-organize output — runtime target (gitignored)

```



See [docs/](docs/) for architecture and roadmap.



## Legacy Pascal build



See [README](README) (original FPC/SDL instructions).

