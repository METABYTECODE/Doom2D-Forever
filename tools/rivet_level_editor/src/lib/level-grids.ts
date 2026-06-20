import type { LevelData } from "../types/level";
import { FLUID_NONE } from "../types/level";
import { clampMapSizePx, emptyCollision, ensureLevelCollision } from "./level-collision";
import { emptyFluids, ensureLevelFluids } from "./level-fluids";
import { subGridCols, subGridRows, snapGridSize } from "./sub-grid";

export function ensureLevelGrids(level: LevelData): LevelData {
  return ensureLevelFluids(ensureLevelCollision(level));
}

function resampleSubGrid(
  oldGrid: number[][],
  widthPx: number,
  heightPx: number,
  oldGridSize: number,
  newGridSize: number,
  emptyValue = 0,
): number[][] {
  const cols = subGridCols(widthPx, newGridSize);
  const rows = subGridRows(heightPx, newGridSize);
  return Array.from({ length: rows }, (_, y) =>
    Array.from({ length: cols }, (_, x) => {
      const ox = Math.floor((x * newGridSize) / oldGridSize);
      const oy = Math.floor((y * newGridSize) / oldGridSize);
      return oldGrid[oy]?.[ox] ?? emptyValue;
    }),
  );
}

export function clampGridSize(value: number): number {
  if (!Number.isFinite(value)) return 16;
  return Math.min(128, Math.max(1, Math.round(value)));
}

/** Change snap/collision sub-grid step; resamples collision + fluids layers. */
export function setGridSize(level: LevelData, gridSizePx: number): LevelData {
  const nextGrid = clampGridSize(gridSizePx);
  const oldGrid = snapGridSize(level);
  if (nextGrid === oldGrid) {
    return level.grid_size === nextGrid ? level : { ...level, grid_size: nextGrid };
  }

  const collision = resampleSubGrid(level.collision, level.width, level.height, oldGrid, nextGrid);
  const fluids = resampleSubGrid(level.fluids, level.width, level.height, oldGrid, nextGrid, FLUID_NONE);

  return ensureLevelGrids({
    ...level,
    grid_size: nextGrid,
    collision,
    fluids,
  });
}

export function resizeLevel(level: LevelData, widthPx: number, heightPx: number): LevelData {
  const nextWidth = clampMapSizePx(widthPx);
  const nextHeight = clampMapSizePx(heightPx);
  const gridSize = snapGridSize(level);
  const cols = subGridCols(nextWidth, gridSize);
  const rows = subGridRows(nextHeight, gridSize);

  const tile_layers = level.tile_layers.map((layer) => ({
    ...layer,
    tiles: layer.tiles.filter((tile) => tile.x < nextWidth && tile.y < nextHeight),
  }));

  const collision = Array.from({ length: rows }, (_, y) =>
    Array.from({ length: cols }, (_, x) => level.collision[y]?.[x] ?? 0),
  );

  const fluids = Array.from({ length: rows }, (_, y) =>
    Array.from({ length: cols }, (_, x) => level.fluids[y]?.[x] ?? 0),
  );

  return ensureLevelGrids({
    ...level,
    width: nextWidth,
    height: nextHeight,
    tile_layers,
    collision,
    fluids,
  });
}
