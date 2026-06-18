import type { LevelData } from "../types/level";

export function clampMapSize(value: number): number {
  if (!Number.isFinite(value)) return 4;
  return Math.min(256, Math.max(4, Math.round(value)));
}

/** Keeps tile grid dimensions in sync with width/height metadata. */
export function ensureLevelTiles(level: LevelData): LevelData {
  const width = clampMapSize(level.width);
  const height = clampMapSize(level.height);

  if (
    level.tiles.length === height &&
    level.tiles.every((row) => row.length === width)
  ) {
    if (level.width === width && level.height === height) {
      return level;
    }
    return { ...level, width, height };
  }

  const tiles = Array.from({ length: height }, (_, y) =>
    Array.from({ length: width }, (_, x) => level.tiles[y]?.[x] ?? 0),
  );

  return { ...level, width, height, tiles };
}

export function resizeLevel(level: LevelData, width: number, height: number): LevelData {
  return ensureLevelTiles({
    ...level,
    width: clampMapSize(width),
    height: clampMapSize(height),
  });
}
