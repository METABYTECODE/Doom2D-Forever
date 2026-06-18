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

/** Grid cells along a line (Bresenham), inclusive of both endpoints. */
export function gridLineCells(
  x0: number,
  y0: number,
  x1: number,
  y1: number,
): Array<{ x: number; y: number }> {
  const cells: Array<{ x: number; y: number }> = [];
  let x = x0;
  let y = y0;
  const dx = Math.abs(x1 - x0);
  const dy = Math.abs(y1 - y0);
  const sx = x0 < x1 ? 1 : -1;
  const sy = y0 < y1 ? 1 : -1;
  let err = dx - dy;

  while (true) {
    cells.push({ x, y });
    if (x === x1 && y === y1) break;
    const e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x += sx;
    }
    if (e2 < dx) {
      err += dx;
      y += sy;
    }
  }

  return cells;
}

export function paintCollisionCells(
  collision: number[][],
  cells: Array<{ x: number; y: number }>,
  value: number,
  width: number,
  height: number,
): boolean {
  let changed = false;
  for (const { x, y } of cells) {
    if (x < 0 || y < 0 || x >= width || y >= height) continue;
    if ((collision[y]?.[x] ?? 0) === value) continue;
    collision[y][x] = value;
    changed = true;
  }
  return changed;
}
