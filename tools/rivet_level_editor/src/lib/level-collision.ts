import type { LevelData } from "../types/level";
import { subGridCols, subGridRows, snapGridSize } from "./sub-grid";

export function clampMapSizePx(value: number): number {
  if (!Number.isFinite(value)) return 64;
  return Math.min(8192, Math.max(64, Math.round(value)));
}

export function emptyCollision(cols: number, rows: number): number[][] {
  return Array.from({ length: rows }, () => Array<number>(cols).fill(0));
}

/** Collision sub-grid matches level px bounds at grid_size resolution. */
export function ensureLevelCollision(level: LevelData): LevelData {
  const gridSize = snapGridSize(level);
  const widthPx = clampMapSizePx(level.width);
  const heightPx = clampMapSizePx(level.height);
  const cols = subGridCols(widthPx, gridSize);
  const rows = subGridRows(heightPx, gridSize);

  if (
    level.collision.length === rows &&
    level.collision.every((row) => row.length === cols) &&
    level.width === widthPx &&
    level.height === heightPx
  ) {
    return level;
  }

  const collision = Array.from({ length: rows }, (_, y) =>
    Array.from({ length: cols }, (_, x) => level.collision[y]?.[x] ?? 0),
  );

  return { ...level, width: widthPx, height: heightPx, collision };
}

export function paintCollisionBorder(level: LevelData): LevelData {
  const { cols, rows } = {
    cols: subGridCols(level.width, snapGridSize(level)),
    rows: subGridRows(level.height, snapGridSize(level)),
  };
  const collision = level.collision.map((row) => [...row]);
  for (let x = 0; x < cols; x++) {
    collision[0][x] = 1;
    collision[rows - 1][x] = 1;
  }
  for (let y = 1; y < rows - 1; y++) {
    collision[y][0] = 1;
    collision[y][cols - 1] = 1;
  }
  return { ...level, collision };
}

/** @deprecated use snapGridSize from sub-grid */
export function worldCellSize(level: LevelData): number {
  return snapGridSize(level);
}

/** Sub-grid cells along a line (Bresenham). */
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
  cols: number,
  rows: number,
): boolean {
  let changed = false;
  for (const { x, y } of cells) {
    if (x < 0 || y < 0 || x >= cols || y >= rows) continue;
    if ((collision[y]?.[x] ?? 0) === value) continue;
    collision[y][x] = value;
    changed = true;
  }
  return changed;
}
