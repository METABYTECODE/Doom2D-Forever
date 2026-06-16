# 04 — Карта модулей: Pascal → C++

Полное соответствие файлов оригинального проекта и целевых C++ модулей.

---

## 1. Entry Point & Application

| Pascal | C++ Target | Описание |
|--------|------------|----------|
| `Doom2DF.lpr` | `src/d2df_client/main.cpp` | Client entry point |
| — | `src/d2df_server/main.cpp` | Dedicated server entry |
| `g_main.pas` | `src/d2df/app/game_app.cpp` | Bootstrap, paths, Init/Release |
| `g_window.pas` | `src/d2df/app/window_loop.cpp` | Main loop, fixed timestep |
| `g_system.pas` + `sdl2/system.inc` | `src/d2df/platform/sdl_platform.cpp` | SDL window, events, ticks |
| `g_touch.pas` | `src/d2df/platform/touch_input.cpp` | Mobile touch (deferred) |
| `g_options.pas` | `src/d2df/app/default_cvars.cpp` | Default CVars |

---

## 2. Game Core

| Pascal | C++ Target | Строк | Описание |
|--------|------------|------:|----------|
| `g_game.pas` | **Decompose into:** | 7 909 | |
| | `src/d2df/app/states/in_game_state.cpp` | | State machine, game flow |
| | `src/d2df/ecs/systems/game_rules_system.cpp` | | Game modes, scoring, limits |
| | `src/d2df/ecs/systems/hud_system.cpp` | | HUD rendering |
| | `src/d2df/ecs/systems/intermission_system.cpp` | | Level transitions |
| | `src/d2df/domain/game_settings.hpp` | | TGameSettings → struct |
| `g_basic.pas` | `src/d2df/domain/collision_utils.cpp` | 891 | AABB, angles, UID |
| `g_console.pas` | `src/d2df/ui/console/console.cpp` | 2 219 | Console, CVars, binds |
| `g_language.pas` | `src/d2df/ui/localization.cpp` | 2 042 | .lng file loader |

---

## 3. ECS Entities & Systems

| Pascal | C++ Components | C++ Systems | Строк |
|--------|---------------|-------------|------:|
| `g_player.pas` | `PlayerTag`, `Health`, `Inventory`, `InputState`, `WeaponState`, `AnimationState` | `PlayerMovementSystem`, `PlayerCombatSystem`, `PlayerAnimationSystem`, `BotAISystem` | 7 189 |
| `g_monsters.pas` | `MonsterType`, `AIState`, `Health` | `MonsterAISystem`, `MonsterCombatSystem`, `MonsterSpawnSystem` | 4 336 |
| `g_weapons.pas` | `ProjectileType`, `OwnerId`, `Lifetime`, `Damage` | `WeaponSystem`, `ProjectileSystem`, `ExplosionSystem` | 2 394 |
| `g_items.pas` | `ItemType`, `RespawnTimer` | `ItemSystem`, `PickupSystem`, `ItemRespawnSystem` | 952 |
| `g_panel.pas` | `PanelType`, `MoveState`, `Size`, `TextureId` | `PanelMovementSystem`, `PanelCollisionSystem` | 1 062 |
| `g_triggers.pas` | `TriggerType`, `TriggerData`, `Enabled` | `TriggerSystem` (+ Exoma) | 3 133 |
| `g_gfx.pas` | `ParticleType`, `Lifetime`, `Color` | `ParticleSystem` | 1 543 |
| `g_playermodel.pas` | `ModelId`, `AnimationFrame`, `SkinData` | `ModelRenderSystem` | 1 090 |
| `g_phys.pas` | `Transform`, `Velocity`, `Collider` | `PhysicsSystem` | 638 |

---

## 4. Map & World

| Pascal | C++ Target | Строк | Описание |
|--------|------------|------:|----------|
| `g_map.pas` | `src/d2df/domain/map/map_data.hpp` | 2 958 | Map data structure |
| | `src/d2df/domain/map/map_loader.cpp` | | Load binary/text maps |
| | `src/d2df/domain/map/map_collision.cpp` | | Collision queries |
| | `src/d2df/render/map_renderer.cpp` | | Layer-ordered rendering |
| `g_grid.pas` | `src/d2df/domain/spatial_grid.cpp` | 1 746 | 32×32 spatial hash |
| `g_textures.pas` | `src/d2df/resources/texture_registry.cpp` | 787 | Texture/animation registry |
| `g_saveload.pas` | `src/d2df/domain/save_game.cpp` | 419 | DFSV v7 format |

---

## 5. MapDef & Code Generation

| Pascal | C++ Target | Описание |
|--------|------------|----------|
| `mapdef/mapdef.txt` | `src/mapdef/mapdef.txt` | **Same file** — schema source |
| `mapgen.dpr` | `tools/mapgen/main.cpp` | Code generator |
| `mapdef.inc` | `generated/mapdef_types.hpp` | Auto-generated enums/structs |
| `mapdef_impl.inc` | `generated/mapdef_parser.hpp` | Auto-generated parser |
| `mapdef_help.inc` | `generated/mapdef_help.hpp` | Auto-generated help strings |
| `mapdef_tgc_def.inc` | `generated/mapdef_trigger_cache.hpp` | Trigger cache vars |
| `mapdef_tgc_impl.inc` | (included in parser) | Trigger cache impl |
| `MAPDEF.pas` | `src/d2df/domain/map/map_def.cpp` | Runtime map helpers |
| `xdynrec.pas` | `src/d2df/domain/map/dynamic_record.cpp` | Dynamic record system |

---

## 6. Engine Layer

| Pascal | C++ Target | Строк | Описание |
|--------|------------|------:|----------|
| `e_graphics.pas` | `src/d2df/render/gl_renderer.cpp` | ~2 145 | OpenGL renderer |
| | `src/d2df/render/sprite_batch.cpp` | | Batch sprite drawing |
| | `src/d2df/render/font_renderer.cpp` | | Font rendering |
| | `src/d2df/render/camera.cpp` | | 2D camera |
| `e_texture.pas` | `src/d2df/resources/texture_loader.cpp` | ~300 | Image → GPU texture |
| `e_input.pas` + `e_input_sdl2.inc` | `src/d2df/platform/input_manager.cpp` | ~400 | Keyboard/mouse/gamepad |
| `e_res.pas` | `src/d2df/resources/resource_manager.cpp` | 375 | Multi-dir resource search |
| `e_log.pas` | `src/d2df/core/log.cpp` | ~200 | Logging (→ spdlog) |
| `e_msg.pas` | `src/d2df/net/net_message.cpp` | ~300 | Binary message codec |
| `e_sound.pas` + backends | `src/d2df/audio/audio_engine.cpp` | ~600 | Sound engine |
| `e_soundfile*.pas` | `src/d2df/audio/sound_loaders/*.cpp` | ~800 | WAV, OGG, MOD, MP3 loaders |

---

## 7. Resources & VFS

| Pascal | C++ Target | Описание |
|--------|------------|----------|
| `wadreader.pas` | `src/d2df/resources/wad/wad_reader.cpp` | WAD API (GetResource, etc.) |
| `sfs.pas` | `src/d2df/resources/vfs/vfs.cpp` | Virtual filesystem core |
| `sfsZipFS.pas` | `src/d2df/resources/vfs/dfwad_volume.cpp` | DFWAD format |
| | `src/d2df/resources/vfs/zip_volume.cpp` | ZIP format |
| `sfsPlainFS.pas` | `src/d2df/resources/vfs/pak_volume.cpp` | Quake PAK format |
| | `src/d2df/resources/vfs/composite_vfs.cpp` | Multi-dir search |
| `g_res_downloader.pas` | `src/d2df/net/wad_downloader.cpp` | WAD download client |

---

## 8. Networking

| Pascal | C++ Target | Строк | Описание |
|--------|------------|------:|----------|
| `g_net.pas` | `src/d2df/net/net_host.cpp` | 2 301 | ENet host/client lifecycle |
| | `src/d2df/net/net_client.cpp` | | Client-specific logic |
| `g_netmsg.pas` | `src/d2df/net/net_protocol.cpp` | 2 997 | Message serialize/deserialize |
| | `src/d2df/net/handlers/host_handlers.cpp` | | Server message handlers |
| | `src/d2df/net/handlers/client_handlers.cpp` | | Client message handlers |
| `g_nethandler.pas` | `src/d2df/net/net_dispatcher.cpp` | 190 | Packet deframing + dispatch |
| `g_netmaster.pas` | `src/d2df/net/net_master.cpp` | 1 839 | Master server client |
| | `src/d2df/net/net_ping.cpp` | | UDP "DF" ping |
| `mastersrv/master.c` | `src/d2df_master/main.cpp` | ~500 | Master server (port from C) |

---

## 9. UI

| Pascal | C++ Target | Строк | Описание |
|--------|------------|------:|----------|
| `g_gui.pas` | `src/d2df/ui/gui/gui_framework.cpp` | 3 130 | Widget hierarchy |
| | `src/d2df/ui/gui/widgets/*.cpp` | | Button, menu, editbox, scrollbar |
| `g_menu.pas` | `src/d2df/ui/menus/*.cpp` | 3 678 | All menu screens |
| `g_panel.pas` (UI) | — | | Not UI — map panel entity |
| `flexui/*` | Deferred (Phase 9+) | ~3 000 | Holmes debugger only |

---

## 10. Shared Utilities

| Pascal | C++ Target | Описание |
|--------|------------|----------|
| `utils.pas` | `src/d2df/core/utils.cpp` | String utils, path helpers |
| `geom.pas` | `src/d2df/core/geometry.hpp` | Points, rects, vectors |
| `mempool.pas` | `src/d2df/core/memory_pool.hpp` | Pool allocator (optional with modern allocators) |
| `idpool.pas` | `src/d2df/core/id_pool.hpp` | ID recycling |
| `binheap.pas` | `src/d2df/core/binary_heap.hpp` | Priority queue for draw sorting |
| `hashtable.pas` | STL `std::unordered_map` | — |
| `fhashdb.pas` | `src/d2df/core/file_hash_db.cpp` | File hash cache |
| `xparser.pas` | `src/d2df/core/text_parser.cpp` | Text parser (for exoma, maps) |
| `exoma.pas` | `src/d2df/domain/scripting/exoma_runtime.cpp` | Expression language |
| `conbuf.pas` | `src/d2df/core/console_buffer.cpp` | Console output buffer |
| `xprofiler.pas` | `src/d2df/core/profiler.hpp` | Simple profiler (→ Tracy optional) |
| `envvars.pas` | `src/d2df/core/env_vars.cpp` | Environment variables |
| `CONFIG.pas` | `src/d2df/core/config.cpp` | Config file I/O |
| `CONFIGSIMPLE.pas` | (merged into config.cpp) | Simple config |
| `a_modes.inc` | CMake options | Compile flags → CMake |

---

## 11. Tools

| Pascal/C | C++ Target | Описание |
|----------|------------|----------|
| `mapgen.dpr` | `tools/mapgen/main.cpp` | Generate mapdef headers |
| `mapcvt.dpr` | `tools/mapcvt/main.cpp` | Convert map binary ↔ text |
| `png2map/png2map.c` | `tools/png2map/main.cpp` | PNG → binary map |
| — | `tools/wad_list/main.cpp` | List WAD contents (new) |
| — | `tools/wad_extract/main.cpp` | Extract WAD entries (new) |

---

## 12. Third-party (не портируем, используем C/C++ libs)

| Pascal binding | C/C++ replacement |
|----------------|-------------------|
| `lib/sdl2/*` | SDL3 (or SDL2) |
| `lib/enet/*` | ENet C library |
| `lib/openal/*` | OpenAL-Soft |
| `lib/vampimg/*` | stb_image (+ optional libpng/jpeg) |
| `lib/vorbis/*` | libvorbis + libogg |
| `lib/opus/*` | libopus |
| `lib/modplug/*` | libmodplug |
| `lib/xmp/*` | libxmp |
| `lib/mpg123/*` | libmpg123 (or minimp3) |
| `lib/gme/*` | libgme |
| `lib/fluidsynth/*` | fluidsynth |
| `lib/fmod/*` | miniaudio (preferred) or OpenAL |
| `lib/miniupnpc/*` | miniupnpc (deferred) |
| `lib/libjit/*` | Not needed initially |

---

## 13. Complexity Matrix

Модули отсортированы по сложности порта:

| Сложность | Модули | Стратегия |
|-----------|--------|-----------|
| **Low** | g_window, g_system, g_options, g_touch, e_log, e_res, utils, geom, idpool, binheap | Direct port, modernize API |
| **Medium** | g_phys, g_grid, g_textures, g_items, g_panel, g_gfx, g_sound, g_saveload, wadreader, sfs/*, e_texture, e_input, e_msg | Port + unit tests |
| **High** | g_map, g_weapons, g_playermodel, g_console, g_gui, g_net*, g_language | Port + integration tests |
| **Very High** | g_game, g_player, g_monsters, g_triggers, g_menu, exoma | Decompose into ECS + extensive testing |

---

## 14. Globals → ECS Migration Guide

### g_game.pas globals

| Global | ECS equivalent |
|--------|---------------|
| `gPlayer1`, `gPlayer2` | Entities with `PlayerTag{index=0/1}` |
| `gState` | `GameApp` state machine |
| `gGameSettings` | `GameSession` service struct |
| `gTime` | `FixedTimestepClock` service |
| `gTriggers[]` | Entities with `TriggerType` component |
| `Projectiles[]` | Entities with `ProjectileType` component |
| `ggItems[]` | Entities with `ItemType` component |

### g_player.pas → components

| TPlayer field | Component |
|---------------|-----------|
| `.x, .y, .oldX, .oldY` | `Transform` |
| `.vx, .vy, .ax, .ay` | `Velocity` |
| `.health, .armor` | `Health` |
| `.weapons[], .ammo[]` | `Inventory` |
| `.keys, .dir, .state` | `InputState` / `PlayerState` |
| `.model` | `ModelId` |
| `.obj` (TObj) | `Collider` + `Transform` + `Velocity` |
| `.needSend` | `NetworkId.dirty` |

### g_monsters.pas → components

| TMonster field | Component |
|----------------|-----------|
| `.x, .y` | `Transform` |
| `.type` | `MonsterType` |
| `.state, .target` | `AIState` |
| `.health` | `Health` |
| `.obj` | `Collider` + `Transform` + `Velocity` |
| `.mProxyId` | `SpatialGridHandle` |

---

## 15. Files NOT to port

| File/Dir | Reason |
|----------|--------|
| `src/lib/vampimg/JpegLib/*` | Use stb_image or libjpeg |
| `src/lib/sdl2/*.inc` | Use SDL C/C++ API directly |
| `src/nogl/*` | Not needed with real renderer |
| `src/flexui/*` | Debug tool, defer to Phase 9+ |
| `src/game/g_holmes.*` | Debug tool, defer |
| `src/shared/tests/*` | Rewrite as Catch2 tests |
| `src/lib/libjit/*` | Not used in core gameplay |
| `*.lpi`, `*.lpr` project files | Replaced by CMake |
| `rpm/d2df.spec` | Create new packaging |
