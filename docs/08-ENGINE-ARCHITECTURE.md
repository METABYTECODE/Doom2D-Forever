# 08 — Engine vs Game (OOP + ECS)

## Цель

Отделить **движок** (карта, тайлы, коллизии, базовое движение) от **игры** (Doom-специфика: оружие, триггеры, двери, HUD, intermission, save).

Игра строится **вокруг** движка. Legacy карты и ресурсы — временный источник для парсинга; runtime движка работает с **нативным форматом**.

## Слои

```
┌────────────────────────────────────────────────────────────┐
│  Game client (d2df_app → d2df.exe)                         │
│  Legacy gameplay sim (monsters, weapons, old triggers)     │
│  Engine tile render via engine::MapSession                   │
├────────────────────────────────────────────────────────────┤
│  Engine bridge (d2df_game)                                 │
│  MapPlaySession = load map + tiles + movement only         │
├────────────────────────────────────────────────────────────┤
│  Engine runtime (d2df_engine)                              │
│  MapSession, TileMapRenderer, CollisionWorld, ECS           │
├────────────────────────────────────────────────────────────┤
│  Legacy import (временно)                                  │
│  JSON в assets/content/maps → TileMap                      │
└────────────────────────────────────────────────────────────┘
```

**Сейчас:** `d2df_engine.exe` и `MapPlaySession` — только тайлы + бег. `d2df.exe` по-прежнему использует legacy gameplay, но **не через d2df_game**.

**Позже:** свой формат `d2df-tilemap`, новая trigger system, редактор.

**OOP:** `MapSession` — façade (load, tick, accessors).

**ECS:** `EngineWorld` + `entt::registry` — player + map entities (Transform, Collider, tags).

## Нативный формат карт: `d2df-tilemap`

Файл JSON с `"format": "d2df-tilemap"`. Основные поля:

| Поле | Описание |
|------|----------|
| `world` | `{ "width", "height" }` — размер карты в пикселях |
| `tileset` | Список `{ "asset", "width", "height", "animated" }` |
| `layers` | Упорядоченные слои с `tiles[]` |
| `spawns` | `{ "id", "x", "y" }` — player1, player2, dm |
| `entities` | `{ "type", "x", "y" }` — монстры/пропы на карте |

Тайл:

```json
{ "x": 0, "y": 500, "w": 800, "h": 100, "tile": 0, "flags": ["wall"] }
```

`flags` — строки (`wall`, `water`, `step`, `close_door`, …) или числовой bitmask (совместим с legacy `PANEL_*`).

## Загрузка

`map::load_map_file(path)`:

1. Если `"format": "d2df-tilemap"` → native JSON
2. Иначе → legacy `load_map_json_v1` → `tile_map_from_legacy()`

Playground и тесты используют **TileMap** как единственный runtime тип.

## Engine v1 scope

| Включено | Не включено (позже) |
|----------|---------------------|
| `TileMap` + JSON load/write | Legacy `TriggerSystem` |
| `CollisionWorld` из тайлов | Combat, monsters, items |
| `MapSession::rebuild_collision()` | HUD, audio, save |
| Игрок: run / jump / вода / кислота | Новая trigger system |
| Entities на карте + AABB collision | Полный `d2df.exe` gameplay в engine |
| `TileMapRenderer` | |
| `MapPlaySession` (engine-only bridge) | |
| `d2df-tilemap-convert` tool | |
| Playground `d2df_engine.exe` | |

## Targets (CMake)

| Target | Описание |
|--------|----------|
| `d2df_engine` | TileMap, collision, ECS runtime, renderer |
| `d2df_engine_playground` → `d2df_engine.exe` | Бег по карте |
| `d2df_client` → `d2df.exe` | Полная игра (legacy path) |

## Публичный API

```cpp
namespace d2df::engine::map {
  struct TileMap;
  TileMap load_map_file(const std::filesystem::path& path);
  TileMap tile_map_from_legacy(const d2df::map::MapDocument& legacy);
}

namespace d2df::engine {
  class MapSession {
    void load(map::TileMap map);
    void fixed_tick(MovementInput input);
    const map::TileMap& tile_map() const;
    const physics::CollisionWorld& collision() const;
    EngineWorld& world();
  };
}
```

## Дорожная карта

1. **v1 (текущий)** — TileMap, native JSON load/write, legacy adapter, playground, converter tool.
2. **v2** — sidecar `map.tilemap.json` loaded automatically via `load_map_file()`; batch convert with `d2df-tilemap-convert --batch`.
3. **v3** — динамические тайлы (двери) через `enabled` + `MapSession::rebuild_collision()`.
4. **v4** — web/native map editor → `d2df-tilemap` JSON.
5. **Game layer** — миграция MapViewer на MapSession + game modules.

## Редактор карт (позже)

Web UI редактирует `d2df-tilemap` JSON. `d2df_engine.exe --map ...` для playtest. Свои тайлсеты в `assets/content/`, не legacy WAD.
