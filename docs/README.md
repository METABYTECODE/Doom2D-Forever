# Doom2D Forever — C++ Rewrite Documentation

Документация по аудиту оригинального Pascal-проекта и плану переписывания на C++ с современной архитектурой (OOP + ECS).

## Содержание

| Документ | Описание |
|----------|----------|
| [01-PROJECT-AUDIT.md](01-PROJECT-AUDIT.md) | Полный аудит текущего кодовой базы, риски, метрики |
| [02-TARGET-ARCHITECTURE.md](02-TARGET-ARCHITECTURE.md) | Целевая архитектура C++: слои, ECS, OOP, пайплайн |
| [03-WORK-PLAN.md](03-WORK-PLAN.md) | Поэтапный план работ с приоритетами и критериями готовности |
| [04-FILE-MAPPING.md](04-FILE-MAPPING.md) | Карта Pascal-модулей → C++ модулей |
| [05-FORMAT-SPEC.md](05-FORMAT-SPEC.md) | Спецификация **legacy** форматов (DFWAD, карты) — для import tools |
| [06-REVISED-STRATEGY.md](06-REVISED-STRATEGY.md) | **Актуальная стратегия:** modern assets, MP позже, без Pascal-compat |
| **[07-PROJECT-STATUS.md](07-PROJECT-STATUS.md)** | **Handoff:** что сделано, что нет, следующий шаг (читать первым!) |

## Контекст проекта

- **Оригинал:** Doom2D (1996, Prikol Software) — 2D-платформер в духе Doom
- **Текущая кодовая база:** Doom2D Forever (D2DF-SDL) — FreePascal + SDL2/OpenGL + ENet (reference)
- **Цель:** современный rewrite на C++20/23 с ECS, новым asset pipeline, single-player first; multiplayer — позже

## Быстрые факты

| Параметр | Значение |
|----------|----------|
| Язык (текущий) | Object Pascal (FreePascal ≥ 3.1.1) |
| Строк кода (game+engine+shared) | ~65 000+ |
| Тик симуляции | 36 UPS (~27.8 ms) |
| Legacy ресурсы | DFWAD (zlib) + ZIP (.dfz) в `assets/legacy/` |
| Modern ресурсы | PNG/OGG + manifest.json в `assets/content/` |
| Мультиплеер | Phase 9 (новый протокол, не Pascal v188) |
| Лицензия | GPL-3.0 |

## Текущий порядок работ

> **Статус на 2026-06-14:** Phase 0 + 1a + 1b **готовы**. Следующее — Phase 2b.  
> Подробности: **[07-PROJECT-STATUS.md](07-PROJECT-STATUS.md)**

1. ~~**Phase 0**~~ — CMake, SDL, интерфейсы ✅
2. ~~**Phase 1a**~~ — `d2df-extract`, nested textures, ffmpeg ✅
3. ~~**Phase 1b**~~ — `d2df-organize`, manifest + aliases ✅
4. **Phase 2b** — map import, AssetDatabase ← **сейчас**
5. **Phase 3+** — игра (single-player)
6. **Phase 9** — multiplayer, когда SP stable
