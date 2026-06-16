# 03 — План работ (Roadmap, revised)

> **Обновлено:** учтены решения из [06-REVISED-STRATEGY.md](06-REVISED-STRATEGY.md):
> - без wire-совместимости с Pascal;
> - инструменты миграции ресурсов **до** игрового runtime;
> - мультиплеер отложен, интерфейсы — с Phase 0.

---

## Обзор фаз

```
Phase 0     Phase 1        Phase 2         Phase 3–8              Phase 9
 Infra   →  Import Tools → Content Pipe  →  Game (SP)         →  Multiplayer
                              ↓
                         modern assets/
```

**Single-player release:** ~8–11 месяцев  
**+ Multiplayer:** +2–3 месяца

---

## Phase 0: Инфраструктура (1–2 недели)

### Цель
Build pipeline + архитектурные интерфейсы для будущего MP.

### Задачи

| # | Задача | Приоритет |
|---|--------|-----------|
| 0.1 | CMake: `tools/`, `src/d2df/`, `tests/` | P0 |
| 0.2 | vcpkg: zlib, SDL, glm, Catch2, fmt, spdlog | P0 |
| 0.3 | SDL window stub | P0 |
| 0.4 | `EventBus` (pub/sub для game events) | P0 |
| 0.5 | `ISimulationAuthority` + `LocalSimulationAuthority` | P0 |
| 0.6 | `INetworkTransport` + `NullNetworkTransport` | P0 |
| 0.7 | `NetworkIdentity` ECS component (stub) | P1 |
| 0.8 | CI (build + empty tests) | P1 |
| 0.9 | Структура `assets/legacy/` + `assets/content/` | P0 |

### Deliverables
- [x] Пустое окно, CI green
- [x] Интерфейсы authority/transport зарегистрированы в ServiceRegistry

**Статус: ✅ завершено** (см. [07-PROJECT-STATUS.md](07-PROJECT-STATUS.md))

---

## Phase 1a: Extract + Convert (2–3 недели)

### Цель
WAD/dfz → `assets/staging/`. Свалка, но с нормальными форматами (OGG, PNG).

| # | Задача | Приоритет |
|---|--------|-----------|
| 1.1 | DFWAD + ZIP reader | P0 |
| 1.2 | `d2df-extract` → staging/ (структура как в WAD) | P0 |
| 1.3 | CP1251 → UTF-8, ASCII slug на диске | P0 |
| 1.4 | `staging/manifest.json` | P0 |
| 1.5 | Audio sniff + **ffmpeg** → OGG | P0 |
| 1.6 | Images → PNG | P0 |
| 1.7 | Unknown → `_raw/` + report | P1 |
| 1.8 | `d2df-wad-ls` | P0 |

```bash
d2df-extract --input assets --output assets/staging --ffmpeg ffmpeg
```

**Статус: ✅ завершено** (nested DFWAD anim textures, JPEG, MIDI — см. status doc)

---

## Phase 1b / Phase 2a: Organize (1–2 недели)

### Цель
staging/ → content/ с логическими папками.

| # | Задача | Приоритет |
|---|--------|-----------|
| 2.1 | `d2df-organize` — rules по legacy prefix | P0 |
| 2.2 | `organize_overrides.json` | P1 |
| 2.3 | `content/manifest.json` + aliases | P0 |
| 2.4 | `_unclassified/` для ручной сортировки | P1 |

**Статус: ✅ завершено** (~919 unclassified, нужен re-run extract после nested fix)

---

## Phase 2b: Maps + AssetDatabase (1–2 недели)

### Задачи

| # | Задача | Приоритет |
|---|--------|-----------|
| 2.1 | `AssetDatabase` — load manifest, resolve by ID | P0 |
| 2.2 | Alias resolver (legacy path → asset ID) | P1 |
| 2.3 | Port mapgen: mapdef.txt → C++ structs (legacy parse) | P0 |
| 2.4 | `d2df-map-import` — legacy map → JSON map v1 | P0 |
| 2.5 | Rewrite map resource refs (music, sky, textures) | P0 |
| 2.6 | JSON map schema + validator | P0 |
| 2.7 | Batch convert all maps in assets/legacy/maps/ | P0 |
| 2.8 | `d2df-content-validate` — check refs, missing assets | P1 |
| 2.9 | mapgen for JSON schema (optional codegen) | P2 |

### Критерий готовности
- [x] Все карты из legacy/maps конвертированы (233/233)
- [x] Нет broken asset refs в validation report (0 broken refs)
- [x] `AssetDatabase.get("music.prostota")` возвращает OGG bytes

**Статус: ✅ завершено** (2026-06-16, см. [07-PROJECT-STATUS.md](07-PROJECT-STATUS.md))

---

## Phase 3: Rendering (3–4 недели)

### Цель
Map viewer на converted assets.

### Задачи

| # | Задача | Приоритет |
|---|--------|-----------|
| 3.1 | OpenGL/bgfx renderer | P2 (deferred — SDL backend for port MVP) |
| 3.2 | Texture load from content/textures/ | P0 |
| 3.3 | MapRenderer — panel layers | P0 |
| 3.4 | Camera2D + integer scale | P0 |
| 3.5 | Sky/backdrop | P1 |
| 3.6 | Fixed timestep + interpolation skeleton | P0 |
| 3.7 | Font rendering from content/fonts/ | P1 |

### Deliverables
- [x] Map viewer: load JSON map, fly camera, all layers
- [x] Fixed timestep skeleton (36 UPS)
- [x] Sky parallax, HUD, map cycle

**Статус: ✅ MVP завершён** (SDL2 backend; OpenGL — позже)

---

## Phase 4: Physics & ECS (4–6 недель)

### Цель
Игрок ходит по карте.

**Статус: ✅ завершено** (doors, lifts, traps, acid, exit, EventBus events)

---

## Phase 5: Combat (6–8 недель)

Player, weapons, projectiles. EventBus events для будущей replication:
`PlayerFired`, `EntityDamaged`, `EntityDied`.

---

## Phase 6: World (6–8 недель)

Monsters, items, triggers. Exoma → port или замена на Lua (решение на Phase 6).

---

## Phase 7: UI & Audio (4–6 нед weeks, parallelizable)

Menus, HUD, console, OGG playback from content/.

---

## Phase 8: Polish (4–6 недель)

Bots, save/load (новый format), performance, COOP playthrough.

**Milestone: Single-player feature complete.**

---

## Phase 9: Multiplayer (2–3 месяца, отложено)

### Предусловия
- Phase 8 done
- EventBus покрывает все значимые game events
- `NetworkIdentity` на всех replicated entities

### Задачи

| # | Задача | Приоритет |
|---|--------|-----------|
| 9.1 | Новый протокол (versioned, не Pascal v188) | P0 |
| 9.2 | ENet `INetworkTransport` implementation | P0 |
| 9.3 | Server-authoritative tick | P0 |
| 9.4 | `ReplicationSystem` ← EventBus | P0 |
| 9.5 | Input uplink, state downlink | P0 |
| 9.6 | Client prediction + interpolation | P1 |
| 9.7 | Dedicated server binary | P1 |
| 9.8 | Server browser (новый master protocol) | P2 |
| 9.9 | Content sync (manifest hash, не WAD download) | P1 |

### Архитектурный задел (уже есть с Phase 0)
- `ISimulationAuthority` → swap Local → Server/Client
- `INetworkTransport` → swap Null → ENet
- `NetworkIdentity` component
- Replication-ready events in EventBus

---

## Phase 10: Release

Packaging, docs, beta.

---

## Порядок действий (кратко)

| # | Действие | Когда |
|---|----------|-------|
| 1 | ~~Phase 0: CMake + interfaces~~ | ✅ |
| 2 | ~~Phase 1: extract + organize~~ | ✅ |
| 3 | ~~Phase 2b: map import, AssetDatabase~~ | ✅ |
| 4 | **Phase 4: player + collision** | **← сейчас** |
| 5 | Phase 5–8: combat, UI, audio | После 4 |
| 6 | Phase 9: multiplayer | Когда SP stable |

---

## Definition of Done

### Single-player (Phase 8)
- [ ] Все legacy карты играбельны через converted content
- [ ] DM/COOP/CTF offline (bots)
- [ ] Звук и музыка из content/
- [ ] 60 FPS, no leaks

### Multiplayer (Phase 9, optional milestone)
- [ ] 2+ players over network
- [ ] Dedicated server
- [ ] Content version check via manifest hash

### Не входит в scope
- Wire-compat с Pascal клиентом/сервером
- Runtime DFWAD loading
- WAD download protocol
