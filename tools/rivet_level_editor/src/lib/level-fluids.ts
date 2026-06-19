import type { LevelData } from "../types/level";
import { FLUID_NONE, type FluidId } from "../types/level";
import { clampMapSize } from "./level-collision";

export function emptyFluids(width: number, height: number): number[][] {
  return Array.from({ length: height }, () => Array<number>(width).fill(FLUID_NONE));
}

export function ensureLevelFluids(level: LevelData): LevelData {
  const width = clampMapSize(level.width);
  const height = clampMapSize(level.height);

  if (
    level.fluids.length === height &&
    level.fluids.every((row) => row.length === width)
  ) {
    if (level.width === width && level.height === height) {
      return level;
    }
    return { ...level, width, height };
  }

  const fluids = Array.from({ length: height }, (_, y) =>
    Array.from({ length: width }, (_, x) => level.fluids[y]?.[x] ?? FLUID_NONE),
  );

  return { ...level, width, height, fluids };
}

export function paintFluidCells(
  fluids: number[][],
  cells: Array<{ x: number; y: number }>,
  value: FluidId,
  width: number,
  height: number,
): boolean {
  let changed = false;
  for (const { x, y } of cells) {
    if (x < 0 || y < 0 || x >= width || y >= height) continue;
    if ((fluids[y]?.[x] ?? FLUID_NONE) === value) continue;
    fluids[y][x] = value;
    changed = true;
  }
  return changed;
}
