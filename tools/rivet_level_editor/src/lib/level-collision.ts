import type { LevelData } from "../types/level";
import { GRID_SIZE } from "../types/level";

export function clampMapSize(value: number): number {
  if (!Number.isFinite(value)) return 4;
  return Math.min(512, Math.max(4, Math.round(value)));
}

export function emptyCollision(width: number, height: number): number[][] {
  return Array.from({ length: height }, () => Array<number>(width).fill(0));
}

/** Keeps collision grid dimensions in sync with width/height metadata. */
export function ensureLevelCollision(level: LevelData): LevelData {
  const width = clampMapSize(level.width);
  const height = clampMapSize(level.height);

  if (
    level.collision.length === height &&
    level.collision.every((row) => row.length === width)
  ) {
    if (level.width === width && level.height === height) {
      return level;
    }
    return { ...level, width, height };
  }

  const collision = Array.from({ length: height }, (_, y) =>
    Array.from({ length: width }, (_, x) => level.collision[y]?.[x] ?? 0),
  );

  return { ...level, width, height, collision };
}

export function resizeLevel(level: LevelData, width: number, height: number): LevelData {
  const nextWidth = clampMapSize(width);
  const nextHeight = clampMapSize(height);

  const tiles = level.tiles.filter((tile) => {
    return tile.x < nextWidth && tile.y < nextHeight;
  });

  return ensureLevelCollision({
    ...level,
    width: nextWidth,
    height: nextHeight,
    tiles,
  });
}

export function paintCollisionBorder(level: LevelData): LevelData {
  const collision = level.collision.map((row) => [...row]);
  for (let x = 0; x < level.width; x++) {
    collision[0][x] = 1;
    collision[level.height - 1][x] = 1;
  }
  for (let y = 1; y < level.height - 1; y++) {
    collision[y][0] = 1;
    collision[y][level.width - 1] = 1;
  }
  return { ...level, collision };
}

export function worldCellSize(level: LevelData): number {
  return level.grid_size || GRID_SIZE;
}
