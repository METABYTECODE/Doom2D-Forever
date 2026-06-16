# 06 — Пересмотренная стратегия (2026)

Документ фиксирует решения, которые меняют первоначальный план.

---

## 1. Ключевые решения

| Вопрос | Было в первом плане | **Новое решение** |
|--------|---------------------|-------------------|
| Совместимость с Pascal | Wire-compatible multiplayer с protocol 188 | **Не нужна.** Освежаем проект, делаем современным |
| Ресурсы | Runtime читает DFWAD напрямую | **Конвертируем** legacy → modern format, runtime читает только новый |
| Мультиплеер | Phase 6, interop-тесты с Pascal | **Откладываем.** Закладываем интерфейсы и ECS-компоненты с первого дня |
| Инструменты | Phase 1, параллельно с игрой | **Сначала инструменты** — импорт, распаковка, нормализация имён |
| Кириллица в ресурсах | CP1251 lookup как в Pascal | **Manifest + slug ID** — кириллица только в metadata, не в путях |

---

## 2. Почему инструменты — раньше игры

```
assets/legacy/     staging/ (свалка)     content/ (логика)     Runtime
  game.WAD    ──►  d2df-extract    ──►  d2df-organize   ──►  AssetDatabase
  standart.WAD         │                  │
                       │ ffmpeg→OGG       │ MSOUNDS→audio/sfx/monsters/
                       │ PNG              │ D2DMUS→audio/music/
                       ▼                  ▼
                staging_manifest     content/manifest
```

**Причины делать import tools первыми:**

1. **Русские имена** (`D2DMUS\ПРОСТОТА`, `D2DTEXTURES\...`) — проблема кодировки (CP1251 vs UTF-8) и файловых систем. Решается один раз в tooling, а не в каждой подсистеме игры.
2. **Карты ссылаются на legacy-пути** — при конвертации карты переписываем ссылки или строим alias-table.
3. **CI и тесты** — unpacked assets проще версионировать и diff'ить, чем бинарные WAD.
4. **Игра остаётся простой** — runtime не знает про DFWAD, zlib, `wad:\PATH\NAME`.

**Вывод:** legacy reader нужен в **tools/**, не в **src/d2df/** (кроме опционального fallback для modding).

---

## 3. Мультиплеер: отложить, но заложить

### 3.1 Что делаем сразу (нулевая стоимость на будущее)

```cpp
// ecs/components/network.hpp — компонент есть всегда, в SP просто ignored
struct NetworkIdentity {
    uint32_t net_id = 0;       // 0 = local-only
    uint8_t  owner_peer = 0;   // 0 = host/local
    bool     dirty = false;
};

// core/simulation_authority.hpp
enum class Authority { Local, Server, Client };

class ISimulationAuthority {
public:
    virtual Authority mode() const = 0;
    virtual bool is_authoritative(EntityId e) const = 0;
};

// Single-player: LocalSimulationAuthority — всё authoritative локально
// Multiplayer (позже): ServerSimulationAuthority + ClientPrediction
```

### 3.2 Event bus — replication-ready events

Системы не вызывают сеть напрямую. Публикуют события:

```cpp
struct PlayerFired { EntityId player; WeaponId weapon; Vec2 origin; float angle; };
struct EntityDamaged { EntityId victim; EntityId attacker; int amount; };
struct ItemPickedUp { EntityId item; EntityId player; };
// ...
```

Позже добавляется `ReplicationSystem : IEventSubscriber` — подписывается и сериализует. В single-player subscriber пустой.

### 3.3 Интерфейсы (Phase 0)

```cpp
class INetworkTransport {
public:
    virtual ~INetworkTransport() = default;
    virtual void poll() = 0;
    virtual bool is_connected() const = 0;
    // ...
};

class NullNetworkTransport : public INetworkTransport { /* no-op */ };
```

Игра стартует с `NullNetworkTransport`. ENet подключается в Phase MP.

### 3.4 Что НЕ делаем до Phase MP

- ENet, master server, WAD download
- Client prediction, interpolation для remote players
- Protocol serialization
- Dedicated server binary

### 3.5 Что берём из Pascal как **reference**, не как spec

- Server-authoritative model (идея, не протокол)
- 36 UPS fixed tick (можно оставить или сделать configurable)
- Какие entity state нужно синхронизировать (список из g_netmsg.pas)

---

## 4. Pipeline ресурсов: две стадии

Legacy — «свалка» в WAD (русские имена, разные форматы, хаотичные пути). **Не пытаемся сразу разложить по красивым папкам.** Сначала вытаскиваем и конвертируем форматы, потом — логическая структура.

```
assets/legacy/          Стадия 1: extract          Стадия 2: organize         Runtime
  game.WAD      ──►   assets/staging/      ──►   assets/content/      ──►  AssetDatabase
  standart.WAD         (свалка, но читаемая)      (логические папки)
  maps/*.dfz                │                           │
                            │ ffmpeg, PNG               │ rules + ручные overrides
                            ▼                           ▼
                     staging_manifest.json       content/manifest.json
                     (legacy path → file)        (asset id → file)
```

### 4.1 Стадия 1 — `assets/staging/` (extract + convert)

**Цель:** всё с диска WAD на файловую систему, форматы приведены к playable, структура **повторяет WAD** (или flat + manifest).

```
assets/staging/
├── manifest.json              # каждый файл: legacy_ref, original_name, encoding, format
├── game.WAD/
│   ├── SOUNDS/DOOROPEN.ogg    # было сырьё без ext → ffmpeg
│   ├── MSOUNDS/               # звуки монстров (как в WAD)
│   ├── MTEXTURES/
│   ├── TEXTURES/
│   ├── WEAPONS/
│   └── ...
├── standart.WAD/
│   ├── D2DMUS/prostota.ogg    # slug на диске; meta.title = "Простота"
│   ├── D2DTEXTURES/
│   └── ...
└── maps/                      # распакованные dfz/wad maps (ещё legacy format)
```

**Правила staging:**
- Имена файлов на диске — **ASCII slug** (translit/hash), кириллица только в manifest
- **Форматы конвертируем сразу**, структуру папок не трогаем
- Raw bytes до конвертации можно сохранять в `staging/_raw/` для отладки (опционально)

**Инструмент:** `d2df-extract` (+ `d2df-wad-ls` для inspect)

### 4.2 Стадия 2 — `assets/content/` (organize)

**Цель:** логические директории для runtime и для людей.

```
assets/content/
├── manifest.json
├── aliases.json               # legacy_ref → asset id
├── audio/
│   ├── sfx/world/             # двери, кнопки, ambient карты
│   ├── sfx/ui/                # меню, vote, announcer
│   ├── sfx/monsters/          # из MSOUNDS
│   ├── sfx/weapons/
│   ├── sfx/player/
│   └── music/                 # из D2DMUS, MUSIC
├── textures/
│   ├── tiles/                 # тайлы карт (D2DTEXTURES, STDTEXTURES)
│   ├── monsters/              # MTEXTURES
│   ├── weapons/
│   ├── ui/                    # HUD, menu
│   ├── effects/               # particles, blood
│   └── skies/
├── fonts/
├── models/
│   └── player/doomer/
└── maps/
    └── map01.json
```

**Инструмент:** `d2df-organize` — правила по legacy-пути + ручной `organize_overrides.json`

Пример автоматических правил (из Pascal-кода уже видно разбиение в WAD):

| Legacy prefix | → Content path |
|---------------|----------------|
| `game.WAD:SOUNDS/` | `audio/sfx/world/` |
| `game.WAD:MSOUNDS/` | `audio/sfx/monsters/` |
| `game.WAD:CHATSND/` | `audio/sfx/ui/` |
| `game.WAD:TEXTURES/` (HUD, menu) | `textures/ui/` |
| `game.WAD:MTEXTURES/` | `textures/monsters/` |
| `game.WAD:WEAPONS/` | `textures/weapons/` |
| `standart.WAD:D2DMUS/` | `audio/music/` |
| `standart.WAD:D2DTEXTURES/` | `textures/tiles/` |
| `standart.WAD:D2DSKY/` | `textures/skies/` |

Спорные/непонятные файлы остаются в `content/_unclassified/` до ручной сортировки.

**Инструмент:** `d2df-organize` (Phase 2, после extract)

### 4.3 Конвертация форматов (ffmpeg + image)

В WAD лежит много разных форматов (WAV, MOD/XM/S3M, MP3, OGG, сырые samples без расширения, TGA/PNG/BMP...). **ffmpeg** — основной инструмент для аудио (у тебя уже установлен).

```bash
# Detect → convert → OGG Vorbis (единый target для SFX и music)
ffmpeg -i input.wav  -c:a libvorbis -q:a 6 output.ogg
ffmpeg -i input.mod  -c:a libvorbis -q:a 6 output.ogg   # tracker modules
ffmpeg -i input.mp3  -c:a libvorbis -q:a 6 output.ogg
```

**Pipeline в `d2df-extract`:**
1. Sniff magic bytes (RIFF, OggS, MOD, etc.) — как в Pascal `e_soundfile.pas`
2. Если уже OGG/WAV — ffmpeg в OGG
3. MOD/XM/IT/S3M — ffmpeg (или fallback libxmp → WAV → ffmpeg)
4. Неизвестный — log warning, сохранить raw в `_raw/` для ручной обработки

**Изображения:**
- TGA/PNG/BMP/GIF → **PNG** (stb_image или ffmpeg для exotic)
- Анимации — отдельный manifest flag `animated: true`

**Не тащим в runtime:** FMOD-specific, proprietary — только converted OGG/PNG.

### 4.4 manifest.json (content, финальный)

```json
{
  "version": 1,
  "assets": [
    {
      "id": "music.prostota",
      "type": "music",
      "path": "music/prostota.ogg",
      "legacy_refs": [
        "standart.wad:D2DMUS/ПРОСТОТА",
        "standart.wad:D2DMUS/\\u041f\\u0420\\u041e\\u0421\\u0422\\u041e\\u0422\\u0410"
      ],
      "meta": { "title": "Простота", "original_encoding": "cp1251" }
    }
  ]
}
```

### 4.5 Правила именования (content)

| Правило | Пример |
|---------|--------|
| ID — ASCII slug, dot-separated | `music.prostota`, `tex.hud`, `sfx.door_open` |
| Файлы — ASCII, snake_case | `door_open.ogg`, `stone_01.png` |
| Кириллица — только в `meta.title`, `meta.original_name` | `"title": "Простота"` |
| Формат изображений | PNG (lossless) или WebP |
| Формат звука | OGG Vorbis (SFX и music) |
| Карты | JSON (human-editable) или binary v2 с JSON schema |

### 4.6 Конвертация карт

При импорте карты:
1. Парсим legacy binary/text map
2. Заменяем `music "Standart.wad:D2DMUS\ПРОСТОТА"` → `"music": "music.prostota"`
3. Заменяем texture paths → texture IDs из manifest
4. Сохраняем `maps/map01.json`

Pascal `mapdef.txt` остаётся **reference schema** для понимания бинарного формата. Целевой runtime schema — JSON + codegen.

---

## 5. Обработка кириллицы и legacy encoding

### 5.1 Проблемы

- WAD entry names: CP1251 bytes
- Map strings: заявлен UTF-8 в mapdef, но на практике CP1251
- Windows paths в Pascal: `\` vs `/`
- Case-insensitive lookup (Pascal: `StrEquCI1251`)

### 5.2 Решение в import tool

```
1. Read WAD entry name as bytes
2. Try decode: CP1251 → UTF-8, fallback Latin-1
3. Normalize: lowercase, / slashes, NFC unicode
4. Generate slug: transliterate or hash if non-ASCII
   "ПРОСТОТА" → "prostota" (translit table)
   or "PROSTOTA" if already Latin in WAD
5. Store mapping in aliases.json
6. Log warnings for collisions
```

### 5.3 Transliteration table (минимум)

```
А→a Б→b В→v Г→g Д→d Е→e Ё→yo Ж→zh З→z И→i ...
```

Для уникальности при коллизиях: append `_2`, `_3` или short hash.

---

## 6. Пересмотренный порядок фаз

```
Phase 0   Infra
Phase 1a  d2df-extract       WAD → staging/ + format convert (ffmpeg)
Phase 1b  d2df-wad-ls        inspect, отчёты, _raw/ для проблемных
Phase 2a  d2df-organize      staging/ → content/ (логические папки)
Phase 2b  d2df-map-import    карты + rewrite refs
Phase 3     Render
Phase 4–8   Игра (SP)
Phase 9   Multiplayer
Phase 10  Release
```

**Оценка:** ~8–11 месяцев без multiplayer (Phase 9 отдельно +2–3 месяца).

---

## 7. Что делаем прямо сейчас (следующие шаги)

### Шаг 1 — Phase 0 (1–2 недели)
- [ ] CMake skeleton: `tools/`, `src/d2df/`, `tests/`
- [ ] SDL window stub
- [ ] `ISimulationAuthority`, `INetworkTransport` (null impl)
- [ ] `EventBus` skeleton
- [ ] CI

### Шаг 2 — Phase 1a: Extract + convert (2–3 недели)
- [ ] `d2df-extract` — DFWAD/ZIP → `assets/staging/`
- [ ] CP1251 → UTF-8, ASCII slug на диске
- [ ] `staging/manifest.json` (legacy paths, original names)
- [ ] Audio: sniff format → **ffmpeg** → OGG
- [ ] Images: → PNG
- [ ] `d2df-wad-ls` — inspect
- [ ] Report: unknown formats → `_raw/`

### Шаг 3 — Phase 2a: Organize (1–2 недели)
- [ ] `d2df-organize` — staging → content по rules (MSOUNDS → audio/sfx/monsters/...)
- [ ] `organize_overrides.json` — ручные правки
- [ ] `_unclassified/` для спорных файлов
- [ ] `content/manifest.json` + `aliases.json`

### Шаг 4 — Phase 2b: Maps (1–2 недели)
- [ ] Port mapgen from mapdef.txt (legacy schema)
- [ ] `tools/d2df-map-import` — legacy map → JSON map
- [ ] Rewrite resource refs via aliases
- [ ] Runtime `AssetDatabase` — load from manifest only
- [ ] Validate: все карты из assets/maps/ конвертируются

### Шаг 5 — Phase 3+: Игра
- [ ] Map viewer (render converted map)
- [ ] Physics, player, ... по плану

### Шаг 6 — Multiplayer (когда SP готов)
- [ ] Новый протокол (не v188)
- [ ] ENet transport implements `INetworkTransport`
- [ ] `ReplicationSystem` подписан на EventBus

---

## 8. Ответ на вопрос «инструменты сначала или параллельно?»

**Сначала инструменты (Phase 1–2), потом игра (Phase 3+).**

| Если tools первыми | Если tools параллельно |
|--------------------|------------------------|
| Кириллица решена до game code | Каждый модуль тащит legacy encoding |
| Runtime простой | Два пути загрузки (WAD + modern) |
| Карты уже с нормальными ID | Карты ломаются на Windows/Linux по-разному |
| CI с unpacked assets | CI без ресурсов или с WAD binary |

Минимальный **blocking path**: `d2df-import` → один converted map → map viewer.

---

## 9. Scope: что сохраняем от оригинала

| Сохраняем | Не сохраняем |
|-----------|--------------|
| Gameplay feel (36 UPS, physics values) | Pascal wire protocol |
| Все карты (через conversion) | DFWAD runtime loading |
| Все ресурсы (через conversion) | CP1251 paths в runtime |
| Map entity types (panels, triggers...) | FlexUI / Holmes |
| Game modes (DM, CTF, COOP...) | FMOD, immediate-mode GL |
| Идея server-authoritative MP | WAD download over network |

---

## 10. Риски (обновлённые)

| Риск | Митигация |
|------|-----------|
| Потеря ресурсов при конвертации | Import tool сохраняет raw extract + manifest; diff tool |
| Коллизии slug'ов для русских имён | Translit + hash suffix; ручной override в manifest |
| Карты с exoma scripts | Phase 6; возможна замена на Lua позже |
| Scope creep в import tool | Разделить: extract / organize / map-import — три tools |
| ffmpeg не открывает MOD | libxmp → WAV → ffmpeg; или log + `_raw/` |
