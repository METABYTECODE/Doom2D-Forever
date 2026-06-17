# 07 — Статус проекта (handoff)

> **Обновлено:** 2026-06-17  
> Документ для продолжения работы в новом чате. Читать **первым** после `docs/README.md`.

---

## TL;DR — где мы сейчас

| Фаза | Статус |
|------|--------|
| **Phase 0** — инфраструктура | ✅ **Готово** |
| **Phase 1a** — extract + convert | ✅ **Готово** (есть доработки, см. ниже) |
| **Phase 1b** — organize | ✅ **Готово** (нужен re-run после последних фиксов extract) |
| **Phase 2b** — maps + AssetDatabase | ✅ **Готово** |
| **Phase 3** — map viewer (SDL MVP) | ✅ **Готово** |
| **Phase 4** — physics & player | ✅ **Готово** |
| **Phase 5** — combat (weapons, projectiles) | ✅ **Готово** |
| **Phase 6** — monsters, items, triggers | ✅ **Готово** |
| **Phase 7** — HUD, audio, pause menu | ✅ **Готово** (звук — доработки позже) |
| **Phase 8** — polish | 🔄 **В работе** (perf + leaks; bots/COOP отложены) |

**Следующий шаг:** Phase 8 — performance profiling, leak checks. Bots/COOP/MP — после rewrite.

---

## Быстрый старт (Windows)

```bat
build.bat              REM cmake + Release build
extract_assets.bat     REM assets/ → assets/staging/  (~10 мин с ffmpeg)
organize_assets.bat    REM assets/staging/ → assets/content/
import_maps.bat        REM mapbin → JSON maps + validation
```

Требования: CMake 3.25+, MSVC 2022, vcpkg (`VCPKG_ROOT` или `C:\vcpkg`), ffmpeg в PATH (для конвертации).

---

## Phase 0 — сделано ✅

- CMake: `src/d2df/`, `src/d2df_client/`, `src/d2df_server/`, `tools/`, `tests/`
- vcpkg manifest: SDL2, spdlog, fmt, Catch2, zlib
- `d2df.exe` — SDL2 окно, ESC для выхода
- `d2df_server.exe` — headless stub
- **Core:** `EventBus`, `ServiceRegistry`, `ISimulationAuthority` / `LocalSimulationAuthority`
- **Core:** `INetworkTransport` / `NullNetworkTransport`
- **ECS stub:** `NetworkIdentity` component
- CI: `.github/workflows/build.yml`
- **12 unit-тестов**, все проходят

### Не сделано / отложено

- [ ] `assets/legacy/` как отдельная папка — пользователь кладёт WAD в `assets/data/`, `assets/maps/`, `assets/wads/` (работает)
- [ ] glm в vcpkg — не подключали (не нужен до Phase 3)

---

## Phase 1a — сделано ✅

### Инструменты

| Tool | Путь | Назначение |
|------|------|------------|
| `d2df-wad-ls` | `tools/d2df_wad_ls/` | Просмотр DFWAD/ZIP |
| `d2df-extract` | `tools/d2df_extract/` | Extract всех `.wad`/`.dfz` → `assets/staging/` |
| `d2df_archive` | `tools/lib/d2df_archive/` | Общая библиотека |

### Библиотека `d2df_archive`

| Модуль | Файл | Что делает |
|--------|------|------------|
| DFWAD reader | `archive.cpp` | Парсинг DFWAD, zlib inflate (raw + wrapper fallback) |
| ZIP reader | `archive.cpp` | Local headers для `.dfz`/`.zip` |
| CP1251 | `text_encoding.cpp` | Windows API → UTF-8 |
| Slug | `slug.cpp` | Кириллица → ASCII translit |
| Format sniff | `format_sniff.cpp` | WAV, OGG, PNG, TGA, BMP, GIF, MP3, MAP, **JPEG**, **MIDI** |
| Nested unwrap | `nested_resource.cpp` | Вложенные DFWAD (anim textures) → TGA bytes |
| ffmpeg | `ffmpeg_util.cpp` | CreateProcess на Windows, конвертация OGG/PNG |
| Extract pipeline | `extract_pipeline.cpp` | Рекурсивный extract + manifest |
| Staging manifest | `staging_manifest.cpp` | Чтение + dedupe staging manifest |
| Organize pipeline | `organize_pipeline.cpp` | staging → content |
| JSON utils | `json_util.cpp` | Streaming writer + minimal parser |

### Поведение extract

- Рекурсивно находит `.wad`, `.WAD`, `.dfz`, `.zip`, `.pk3` под `--input`
- Output зеркалит структуру WAD: `staging/data/game.WAD/SOUNDS/dooropen.png`
- Имена на диске — ASCII slug; кириллица в `manifest.json` → `original_name`
- ffmpeg: audio→OGG, images→PNG
- Streaming `manifest.json` (без nlohmann — stack overflow на 4000+ entries)

### Вложенные anim-текстуры (важно!)

Некоторые текстуры (напр. `COL5A0`, `COL5B0`) — **не картинки**, а **мини-DFWAD** внутри entry:

```
COL5A0
  └── DFWAD
       ├── TEXT/ANIM      ← INI: resource=COL5A, framecount=2, framewidth=64...
       └── TEXTURES/COL5A ← TGA sprite sheet
```

Pascal: `g_map.pas` → `CreateAnimTexture`, `isWadData()` + `TWADFile.ReadMemory()`.

**Наш extract:** `unwrap_nested_resource()` вытаскивает TGA → PNG.  
Анимацию **не** воспроизводим — только картинку (sprite sheet).  
`col5a0` / `col5b0` — отдельные фазы анимации (A/B кадры).

### Последний полный extract (до nested/JPEG fix)

```
47 archives, 4205 entries, 3088 converted, 0 errors
manifest: assets/staging/manifest.json (~1.2 MB)
```

**⚠️ Нужен re-run** после фиксов nested textures + JPEG + MIDI:
```bat
extract_assets.bat
organize_assets.bat
```

### Phase 1a — не сделано / частично

- [ ] **1.7** `_raw/` + report для unknown — не реализовано (unknown остаётся `.bin`)
- [ ] Split anim sprite sheet на отдельные кадры — не нужен пока (sheet as PNG достаточно)
- [ ] MIDI → OGG через ffmpeg — sniff есть (`.mid`), конвертация может не срабатывать (ffmpeg + MIDI)
- [ ] MOD/tracker без заголовка — эвристика по path prefix (`D2DMUS/`, `MUS_DOOM/`)
- [ ] ZIP central directory — local headers работают для текущих dfz; edge cases возможны
- [ ] Sidecar `.anim.json` с метаданными кадров — не пишем (не нужен для runtime пока)

---

## Phase 1b — сделано ✅

### Инструмент

| Tool | Путь | Назначение |
|------|------|------------|
| `d2df-organize` | `tools/d2df_organize/` | staging → content с логическими папками |

### Output

```
assets/content/
├── manifest.json     # asset id → path + meta
├── aliases.json      # legacy_ref → asset id
├── audio/sfx/world/  # game.WAD:SOUNDS/
├── audio/sfx/monsters/
├── audio/music/      # standart.WAD:D2DMUS/, game.WAD:MUSIC/
├── textures/tiles/   # standart.WAD:D2DTEXTURES/, *:TEXTURES/
├── textures/ui/      # game.WAD:TEXTURES/
├── textures/skies/
├── maps/legacy/      # kind=map entries
└── _unclassified/    # ~919 entries без правила (model .bin, megawad scripts...)
```

### Правила organize

См. `organize_pipeline.cpp` → `default_rules()`. Wildcard `*:` для map-паков (`TEXTURES/`, `MUS_DOOM/`, `MUS_DOOM2/`).

Overrides: `assets/organize_overrides.json` (пример: `menufont` → `textures/ui/menu`).

Dedupe: при дубликате `legacy_ref` предпочитает `data/` над `wads/`.

### Последний organize (до nested fix в extract)

```
3444 unique / 4205 input, 761 duplicates skipped, 919 unclassified, 0 errors
```

Пример: `standart.WAD:D2DMUS/ПРОСТОТА` → `music.prostota` → `audio/music/prostota.ogg`

### Phase 1b — не сделано

- [ ] Больше правил для `_unclassified/` (model packs, `interscript.bin`, `.dfz` model metadata)
- [ ] Ручная сортировка `_unclassified/` — не начата
- [ ] `organize_overrides.json` — только 1 пример

---

## Phase 2b — сделано ✅

| Задача | Статус |
|--------|--------|
| `AssetDatabase` / `ContentCatalog` — load manifest, resolve by ID | ✅ |
| Alias resolver (legacy path → asset ID, CP1251 + mojibake) | ✅ |
| Port legacy mapbin → C++ structs (`legacy_map.cpp`) | ✅ |
| `d2df-map-import` — batch legacy map → JSON v1 | ✅ |
| Rewrite map refs через aliases + map-relative qualify (`:TEXTURES/...`) | ✅ |
| Built-in texture refs (`_water_0/1/2`) preserved | ✅ |
| `d2df-content-validate` — check refs, missing assets | ✅ |
| JSON maps in `assets/content/maps/{pack}/` | ✅ |

### Результаты (2026-06-16)

```
233 maps converted (0 failed)
3444 assets in catalog
Validation: 0 missing files, 0 broken refs
19 unit tests (18 run + 1 skipped integration)
22 unit tests (21 run + 1 skipped integration) — +FixedTimestep, +MapCollision
```

Пример: `standart.wad:D2DMUS/ПРОСТОТА` → `music.prostota` → `audio/music/prostota.ogg`  
Пример карты: `assets/content/maps/64dm/map01.json` (music/sky/textures → asset IDs)

### Инструменты

| Tool | Путь |
|------|------|
| `d2df-map-import` | `tools/d2df_map_import/` |
| `d2df-content-validate` | `tools/d2df_content_validate/` |
| Runtime library | `src/d2df/src/resources/` (`asset_database`, `content_catalog`, `legacy_ref`) |

### Phase 2b — не сделано / отложено

- [ ] **2.9** mapgen codegen для JSON schema (P2, optional)
- [ ] `_unclassified/` map-pack textures остаются как `raw.{pack}.*` IDs (работает, но не в `tex.tile.*`)

Legacy `.mapbin` остаются в `maps/legacy/`; JSON-карты — в `maps/{pack}/`.

---

## Phase 3 — сделано ✅ (SDL MVP, OpenGL отложен)

| Компонент | Путь |
|-----------|------|
| Map JSON loader | `src/d2df/src/map/map_json_loader.cpp` |
| MapRenderer (layers + tiling + blending) | `src/d2df/src/render/map_renderer.cpp` |
| TextureCache (stb_image) | `src/d2df/src/render/texture_cache.cpp` |
| Camera2D + integer scale | `src/d2df/src/render/camera2d.cpp` |
| Sky parallax | `MapViewer::draw_sky` |
| Fixed timestep 36 UPS | `src/d2df/src/core/fixed_timestep.cpp` |
| HUD text (SDL_ttf, system font fallback) | `src/d2df/src/render/text_renderer.cpp` |
| Map catalog + cycle `[/]` | `src/d2df/src/map/map_catalog.cpp` |
| MapViewer app | `src/d2df/src/app/map_viewer.cpp` |
| Content path resolution | `src/d2df/src/app/content_paths.cpp` |

**Запуск:**
```bat
build.bat
run_game.bat
REM или: build\src\d2df_client\Release\d2df.exe --map assets\content\maps\doom2d\map01.json
```

**Управление:** A/D — бег, Space — прыжок, C — свободная камера, `[/]` — карты, `+/-` — zoom, ESC — выход.

### Phase 3 — отложено

- [ ] **3.1** OpenGL/bgfx (переносим на Phase 5+ или когда понадобится perf)
- [ ] Anim textures runtime (nested WAD frames)
- [ ] TTF шрифты в `content/fonts/` (пока system fallback)

---

## Phase 4 — завершено ✅

| Задача | Статус |
|--------|--------|
| Map collision (wall/closedoor/blockmon AABB) | ✅ |
| PlayerState + port g_Obj_Move (gravity, run, jump) | ✅ |
| Step / water / acid panels | ✅ |
| Spawn from AREA_PLAYERPOINT1/2 | ✅ |
| EnTT GameWorld + Transform/Velocity | ✅ |
| Input → movement (A/D, Space, E use) | ✅ |
| Camera follow player | ✅ |
| Moving panels (doors, lifts, traps, teleports) | ✅ |
| Acid damage + trap crush damage | ✅ |
| Exit trigger → next map | ✅ |
| EventBus: PlayerLanded, liquid, damage, MapExit | ✅ |
| Slope climb | ✅ базово |

---

## Phase 5–7 — завершено ✅

Combat (weapons, projectiles, monster AI), world sim (items, triggers, corpses), HUD, pause menu, SFX/music playback, player sprites/death.

Отложено в Phase 7: консоль, content fonts, scoreboard.

---

## Phase 8 — в работе 🔄

| Задача | Статус |
|--------|--------|
| JSON save/load (quicksave F5/F9) | ✅ v1 |
| **Performance profiling** (FPS, region timings) | ✅ |
| **Resource audit** (textures/audio counts, shutdown log) | ✅ |
| **Main menu** (New game / map select) | ✅ |
| Viewport culling (panels, entities, HUD text cache) | ✅ |
| Panel layer index at map load | ✅ |
| Bots / local COOP | ⏸ отложено (сетевой rewrite) |

**Profiling:** Insert → Debug → **Performance** tab. FPS overlay in-game when enabled.

**Leak check:** Debug `d2df.exe` reports CRT leaks on exit when `D2DF_LEAK_CHECK=ON`.

---

## Структура assets (фактическая)

```
assets/
├── data/           # game.WAD, standart.WAD, editor.WAD, models/, flexui.wad ...
├── maps/           # map .wad / .dfz packs
├── wads/           # дубликаты standart.WAD, shrshade.WAD
├── staging/        # d2df-extract output (gitignored, ~4200 files)
├── staging_test/   # тестовый partial extract (можно удалить)
├── content/        # d2df-organize output (gitignored)
│   ├── manifest.json, aliases.json
│   ├── maps/legacy/   # .mapbin sources
│   └── maps/{pack}/   # JSON maps (d2df-map-import)
└── organize_overrides.json
```

Legacy WAD **не в git** (untracked). CI тесты используют `assets/data/game.WAD` если есть.

---

## Тесты (22 штуки)

```
EventBus, ServiceRegistry, NetworkIdentity
CP1251 decode, DFWAD open/extract (integration)
dedupe staging, organize integration
nested anim texture unwrap (col5a0.bin) [skipped without assets]
AssetDatabase alias resolve + read_bytes
legacy map parse, map JSON rewrite, qualify_map_legacy_ref
JPEG + MIDI format sniff
```

Запуск: `ctest --test-dir build -C Release`

---

## Известные проблемы / tech debt

1. **Re-run extract+organize** нужен после nested/JPEG/MIDI фиксов
2. **919 unclassified** в content — model metadata, megawad scripts, часть map-specific ресурсов
3. **MIDI → OGG** — detect OK, ffmpeg convert не проверен
4. **Дубликаты** standart.WAD в `data/` и `wads/` — dedupe работает, но оба архива всё равно сканируются при extract
5. **manifest legacy_ref** — organize пишет UTF-8 через `original_name`; staging `legacy_ref` может быть mojibake
6. **organize_overrides.json** — parser ищет flat blocks; wrapper `"overrides":[]` не парсится (menufont override не применяется)

---

## Ключевые файлы для следующего агента

| Что | Где |
|-----|-----|
| Roadmap | `docs/03-WORK-PLAN.md` |
| Стратегия | `docs/06-REVISED-STRATEGY.md` |
| DFWAD spec | `docs/05-FORMAT-SPEC.md` |
| Pascal anim textures | `src/game/g_map.pas` → `CreateAnimTexture` |
| Pascal textures | `src/game/g_textures.pas` |
| Extract pipeline | `tools/lib/d2df_archive/src/extract_pipeline.cpp` |
| Nested unwrap | `tools/lib/d2df_archive/src/nested_resource.cpp` |
| AssetDatabase | `src/d2df/include/d2df/resources/asset_database.hpp` |
| Map import | `tools/lib/d2df_archive/src/map_import_pipeline.cpp` |
| Map JSON writer | `tools/lib/d2df_archive/src/map_json.cpp` |
| Content validate | `tools/lib/d2df_archive/src/content_validate.cpp` |

---

## Команды (справочник)

```powershell
# Build
.\build.bat

# Extract (full)
.\extract_assets.bat

# Organize
.\organize_assets.bat

# Import maps (mapbin → JSON + validate)
.\import_maps.bat

# Inspect WAD
.\build\tools\d2df_wad_ls\Release\d2df-wad-ls.exe assets\data\game.WAD

# Tests
ctest --test-dir build -C Release
```

---

## Решения, которые нельзя ломать

- **Нет wire-compat** с Pascal multiplayer/protocol v188
- **Runtime читает только `content/`**, не DFWAD
- **DFWAD reader только в `tools/`**
- **Multiplayer Phase 9** — интерфейсы заложены с Phase 0
- **36 UPS** — reference из Pascal, можно сделать configurable позже
- **Кириллица** — только в manifest meta, пути ASCII slug
