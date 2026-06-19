import type { LevelData } from "../types/level";
import { FLUID_NONE, type FluidId } from "../types/level";
import { clampMapSizePx } from "./level-collision";
import { subGridCols, subGridRows, snapGridSize } from "./sub-grid";

export function emptyFluids(cols: number, rows: number): number[][] {
  return Array.from({ length: rows }, () => Array<number>(cols).fill(FLUID_NONE));
}

export function ensureLevelFluids(level: LevelData): LevelData {
  const gridSize = snapGridSize(level);
  const widthPx = clampMapSizePx(level.width);
  const heightPx = clampMapSizePx(level.height);
  const cols = subGridCols(widthPx, gridSize);
  const rows = subGridRows(heightPx, gridSize);

  if (
    level.fluids.length === rows &&
    level.fluids.every((row) => row.length === cols) &&
    level.width === widthPx &&
    level.height === heightPx
  ) {
    return level;
  }

  const fluids = Array.from({ length: rows }, (_, y) =>
    Array.from({ length: cols }, (_, x) => level.fluids[y]?.[x] ?? FLUID_NONE),
  );

  return { ...level, width: widthPx, height: heightPx, fluids };
}

export function paintFluidCells(
  fluids: number[][],
  cells: Array<{ x: number; y: number }>,
  value: FluidId,
  cols: number,
  rows: number,
): boolean {
  let changed = false;
  for (const { x, y } of cells) {
    if (x < 0 || y < 0 || x >= cols || y >= rows) continue;
    if ((fluids[y]?.[x] ?? FLUID_NONE) === value) continue;
    fluids[y][x] = value;
    changed = true;
  }
  return changed;
}
