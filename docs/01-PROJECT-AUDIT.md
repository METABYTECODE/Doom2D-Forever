# 01 — Аудит проекта Doom2D Forever

## 1. Обзор

Doom2D Forever — это community-driven ремейк классической игры Doom2D (1996) с онлайн-мультиплеером. Текущий репозиторий содержит **исходный код на Pascal** и **legacy-ресурсы в `assets/`** (WAD, карты, модели) — добавлены локально для последующей конвертации в modern format.

### 1.1 Что есть в репозитории

```
Doom2D-Forever/
├── assets/          # Ресурсы (WAD, карты, модели) — не в git
├── docs/            # Документация rewrite (этот набор)
├── man/             # Man-страницы (en/ru)
├── rpm/             # RPM spec
├── src/
│   ├── engine/      # Рендер, звук, ввод, ресурсы (21 файл)
│   ├── flexui/      # UI-фреймворк для отладчика Holmes
│   ├── game/        # Игровая логика (31 модуль, ~55k строк)
│   ├── lib/         # Биндинги: SDL2, ENet, OpenAL, VampImg, FMOD...
│   ├── mapdef/      # Схема формата карт (mapdef.txt)
│   ├── mastersrv/   # Master server на C
│   ├── nogl/        # OpenGL stubs
│   ├── sfs/         # Virtual filesystem (WAD/ZIP/PAK)
│   ├── shared/      # Общие утилиты, MAPDEF, wadreader
│   └── tools/       # mapcvt, mapgen, png2map
└── README           # Инструкции сборки FPC
```

### 1.2 Что отсутствует / вне git

- `game.WAD`, `standart.WAD`, `editor.WAD`, `flexui.wad`, `shrshade.WAD`
- Карты (`assets/maps/*.wad`, `*.dfz`)
- Модели игроков (`assets/data/models/`)
- Сохранения, botlist, banlist

**Стратегия:** `assets/legacy/` — исходники для import tools; `assets/content/` — сгенерированный modern format для runtime. См. [06-REVISED-STRATEGY.md](06-REVISED-STRATEGY.md).

---

## 2. Метрики кодовой базы

### 2.1 Игровые модули (src/game/*.pas)

| Модуль | Строк | Назначение |
|--------|------:|------------|
| `g_game.pas` | 7 909 | Центральный оркестратор: состояние игры, HUD, тики, рендер-слои |
| `g_player.pas` | 7 189 | Игрок: движение, бой, инвентарь, боты, CTF, репликация |
| `g_monsters.pas` | 4 336 | AI монстров, бой, спавн, gibs |
| `g_menu.pas` | 3 678 | Все меню (main, options, browser, save/load) |
| `g_triggers.pas` | 3 133 | Триггеры карт: двери, лифты, скрипты |
| `g_gui.pas` | 3 130 | Собственный retained-mode GUI |
| `g_netmsg.pas` | 2 997 | Сериализация сетевых сообщений |
| `g_map.pas` | 2 958 | Загрузка карт, коллизии, draw lists |
| `g_weapons.pas` | 2 394 | Снаряды, hitscan, взрывы, BFG |
| `g_net.pas` | 2 301 | ENet transport, download, host/client |
| `g_console.pas` | 2 219 | Quake-style консоль, CVars, binds |
| `g_language.pas` | 2 042 | Локализация |
| `g_netmaster.pas` | 1 839 | Master server client, browser |
| `g_grid.pas` | 1 746 | Spatial hash (32×32) |
| `g_gfx.pas` | 1 543 | Частицы, эффекты |
| `g_playermodel.pas` | 1 090 | Модели игроков из WAD |
| `g_main.pas` | 1 063 | Bootstrap, пути, Init/Release |
| `g_panel.pas` | 1 062 | Runtime панели карты |
| `g_items.pas` | 952 | Предметы, respawn |
| `g_basic.pas` | 891 | AABB, UID, утилиты |
| `g_textures.pas` | 787 | Реестр текстур и анимаций |
| `g_phys.pas` | 638 | 2D физика объектов |
| Остальные | ~2 500 | sound, saveload, window, options, touch, holmes... |

**Итого game:** ~55 000 строк Pascal.

### 2.2 Другие слои

| Слой | Файлов | Примерный объём | Роль |
|------|--------|-----------------|------|
| `engine/` | 21 | ~5 000 | Рендер (OpenGL), звук, ввод, текстуры |
| `shared/` | 27 | ~8 000 | MAPDEF, wadreader, geom, utils, exoma |
| `sfs/` | 3 | ~1 500 | Virtual FS |
| `flexui/` | 10 | ~3 000 | Debug UI (Holmes) |
| `lib/` | ~150 | ~50 000+ | Сторонние биндинги (не портируем) |
| `tools/` | 4 | ~2 000 | mapcvt, mapgen, png2map |

---

## 3. Архитектурный анализ (текущий)

### 3.1 Паттерн: «God Module + Procedural Units»

```
Doom2DF.lpr
  └── g_main.Main()
        └── g_window.PerformExecution()    ← главный цикл
              ├── sys_HandleEvents()       ← SDL2
              ├── g_main.Update()          ← фиксированный тик 36 UPS
              │     ├── g_Game_PreUpdate()
              │     ├── g_Net_*_Update()
              │     └── g_Game_Update()
              └── g_main.Draw()
                    └── g_Game_Draw()
                          └── renderMapInternal()  ← 15+ слоёв отрисовки
```

**Ключевые характеристики:**

1. **Глобальное состояние** — сотни `g*` переменных, особенно в `g_game.pas`
2. **Смешанный стиль** — процедурные модули (`g_Map_*`, `g_Weapon_*`) + OOP-классы (`TPlayer`, `TMonster`, `TPanel`)
3. **Не ECS** — сущности как классы/records с прямой связью, spatial grid как оптимизация
4. **Data-driven карты** — `mapdef.txt` → code generation → `mapdef.inc`
5. **Скриптовые триггеры** — `exoma` (собственный expression language)
6. **Условная компиляция** — `a_modes.inc` управляет sound/render/platform флагами

### 3.2 Игровой цикл

| Параметр | Значение |
|----------|----------|
| Simulation rate | 36 UPS (`GAME_TICKS = 36`) |
| UPS interval | ~27.8 ms |
| Render | Отдельно от sim, с lerp (`gLerpFactor`) |
| VSync | Опционально |

**Порядок симуляции (in-play):**
1. `g_Map_Update` — панели, лифты, двери
2. `g_Items_Update`
3. `g_Triggers_Update`
4. `g_Weapon_Update`
5. `g_Monsters_Update`
6. `g_GFX_Update`
7. `g_Player_UpdateAll`

### 3.3 Рендер-пайплайн (слои)

```
Sky/Back → Camera → Back panels → Steps → Items → Projectiles →
Shells → Players → Corpses → Walls → Monsters → Drops → Doors →
Particles → Flags → Acid/Water → Dynamic lights → Ambient → Foreground →
HUD → Console → Menus
```

Каждый слой привязан к `GridTag*` битмаскам в `g_map.pas`.

### 3.4 Сеть

| Аспект | Реализация |
|--------|------------|
| Модель | Authoritative listen-server |
| Транспорт | ENet UDP |
| Протокол | v188, message IDs 100–195 |
| Sync | Server simulates, clients send input + receive state |
| Master | Отдельный C daemon (порт 25665) |
| WAD sync | MD5 + chunked download (8192 bytes) |

### 3.5 Ресурсы

| Формат | Описание |
|--------|----------|
| `.wad` (DFWAD) | Собственный архив: magic `DFWAD\x01`, zlib-сжатие |
| `.dfz` | Обычно ZIP (не DFWAD!) — карты, модели |
| `.map` / binary | Формат карт (см. 05-FORMAT-SPEC.md) |
| Путь ресурса | `archive.wad:\PATH\NAME` |

---

## 4. Зависимости (внешние)

### 4.1 Обязательные для сборки

| Библиотека | Версия | Назначение |
|------------|--------|------------|
| FreePascal | ≥ 3.1.1 | Компилятор |
| SDL2 | 2.0.x | Окно, ввод, events |
| ENet | ≥ 1.3.13 | Сеть |
| OpenGL | 2.x | Рендер (desktop) |

### 4.2 Опциональные (звук)

OpenAL, libvorbis, libopus, libmpg123, libmodplug, libxmp, libgme, fluidsynth, FMOD Ex, SDL_mixer

### 4.3 Встроенные (не портируем напрямую)

- **VampImg** — загрузка изображений (JPEG, PNG, TGA, GIF, DDS...)
- **FlexUI** — debug UI framework
- **Exoma** — expression language для триггеров
- **LibJIT** — JIT (используется ограниченно)

---

## 5. Сильные стороны текущего кода

1. **Работающий multiplayer** с WAD-синхронизацией и master server
2. **Data-driven карты** — `mapdef.txt` как единый источник правды для формата
3. **Модульная (на уровне units) структура** — чёткое разделение game/engine/shared
4. **Spatial grid** — эффективные запросы для монстров/снарядов
5. **Mempool/idpool** — zero-allocation hot paths
6. **Инструменты** — mapcvt, mapgen, png2map для pipeline карт
7. **Кроссплатформенность** — Windows, Linux, Android (частично)

---

## 6. Технический долг и риски

### 6.1 Критические

| # | Проблема | Влияние на rewrite |
|---|----------|-------------------|
| R1 | `g_game.pas` — 7900 строк, god object | Нужна декомпозиция на ECS systems + services |
| R2 | Глобальное состояние повсеместно | Затрудняет тестирование и параллелизацию |
| R3 | Нет unit/integration тестов | Регрессии при порте неизбежны без golden tests |
| R4 | Legacy ресурсы (WAD), кириллица в именах | Import tools первыми; manifest + slug ID |
| R5 | Exoma scripting в триггерах | Нужен порт или совместимый интерпретатор |

### 6.2 Средние

| # | Проблема | Рекомендация |
|---|----------|--------------|
| R6 | Условная компиляция (`{$IFDEF}`) | Заменить на runtime/platform abstractions |
| R7 | CP1251 string handling | Перейти на UTF-8 end-to-end |
| R8 | OpenGL immediate mode | Modern OpenGL/Vulkan или retained renderer |
| R9 | Multiplayer complexity | Отложить; интерфейсы с Phase 0, новый протокол в Phase 9 |
| R10 | Circular deps (g_basic ↔ g_map) | Разрешить через interfaces/events |

### 6.3 Низкие

| # | Проблема |
|---|----------|
| R11 | Holmes debugger (FlexUI) — можно отложить |
| R12 | Android touch controls — отдельная фаза |
| R13 | Headless dedicated server — полезен для CI |

---

## 7. Функциональный scope (что нужно воспроизвести)

### 7.1 Game modes

- DM (Deathmatch)
- TDM (Team Deathmatch)
- CTF (Capture The Flag)
- COOP (Cooperative)
- LMS (Last Man Standing) / Survival

### 7.2 Core gameplay

- 2D platformer physics (гравитация, прыжки, склоны, вода/кислота)
- 8+ видов оружия (chainsaw → BFG)
- 20+ типов монстров с AI
- 30+ типов предметов
- 30+ типов триггеров (двери, лифты, телепорты, скрипты)
- Moving/resizing panels
- Dynamic lights
- Particle effects
- Player models (WAD-based skins)
- Save/load (формат DFSV v7)
- Bots
- Voting system
- RCON

### 7.3 Online

- Listen server (до 24 клиентов)
- Dedicated server (headless)
- Master server browser
- WAD auto-download
- Client prediction + interpolation
- Banlist

### 7.4 UI

- Main menu, options, server browser
- In-game HUD
- Console + CVars + binds
- Chat
- Localization (.lng files)

---

## 8. Рекомендуемый tech stack для C++ rewrite

| Компонент | Рекомендация | Альтернатива |
|-----------|--------------|--------------|
| Стандарт | C++20 (минимум), C++23 (желательно) | — |
| Build | CMake 3.25+ + vcpkg/Conan | Meson |
| ECS | **EnTT** или **flecs** | Custom sparse-set ECS |
| Window/Input | SDL3 или SDL2 | GLFW |
| Render | bgfx / OpenGL 3.3+ / Vulkan | Raylib |
| Audio | miniaudio + dr_libs | OpenAL-Soft |
| Network | ENet (C lib, те же bindings) | — |
| Compression | zlib (raw deflate) | miniz |
| Image | stb_image | libpng + libjpeg-turbo |
| Math | glm | DirectXMath |
| Serialization | Custom (TMsg-compatible) | — |
| Scripting | Port exoma / Lua / Wren | — |
| Testing | Catch2 / Google Test | doctest |
| CI | GitHub Actions | — |
| Format | clang-format | — |
| Docs | Doxygen (API) + markdown (design) | — |

---

## 9. Оценка трудозатрат (грубая)

| Фаза | Описание | Оценка |
|------|----------|--------|
| 0 | Инфраструктура, build, asset pipeline | 2–3 недели |
| 1 | DFWAD/ZIP reader, map loader | 3–4 недели |
| 2 | Render + texture pipeline | 3–4 недели |
| 3 | Physics + map collision | 4–6 недель |
| 4 | Player + weapons | 6–8 недель |
| 5 | Monsters + items + triggers | 6–8 недель |
| 6 | Networking (interop) | 4–6 недель |
| 7 | UI + menus + console | 4–6 недель |
| 8 | Audio | 2–3 недели |
| 9 | Save/load, bots, polish | 4–6 недель |
| 10 | QA, interop testing, release | 4+ недели |

**Итого:** ~10–14 месяцев для команды 1–2 разработчиков при полном feature parity.

---

## 10. Выводы аудита

1. Проект **реалистичен для rewrite**, но объём значительный (~65k строк game logic)
2. **Import tools — первый приоритет** — кириллица, DFWAD, конвертация карт
3. **Wire-compat с Pascal не нужен** — новый протокол и modern assets
4. **ECS — правильный выбор**; multiplayer-ready events с первого дня
5. **mapdef.txt — reference schema** для legacy map import
6. **Exoma — риск Phase 6** — port или замена на Lua
