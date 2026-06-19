import type { LevelData } from "../types/level";
import { GRID_SIZE } from "../types/level";

/** Snap overlay step (px). Collision/fluids sub-grid uses the same step. */
export function snapGridSize(level: LevelData): number {
  return level.grid_size || GRID_SIZE;
}

export function subGridCols(widthPx: number, gridSize: number): number {
  return Math.max(1, Math.ceil(widthPx / gridSize));
}

export function subGridRows(heightPx: number, gridSize: number): number {
  return Math.max(1, Math.ceil(heightPx / gridSize));
}

export function subGridDimensions(level: LevelData): {
  cols: number;
  rows: number;
  gridSize: number;
} {
  const gridSize = snapGridSize(level);
  return {
    cols: subGridCols(level.width, gridSize),
    rows: subGridRows(level.height, gridSize),
    gridSize,
  };
}

export function worldToSubCell(worldX: number, worldY: number, gridSize: number) {
  return {
    x: Math.floor(worldX / gridSize),
    y: Math.floor(worldY / gridSize),
  };
}
