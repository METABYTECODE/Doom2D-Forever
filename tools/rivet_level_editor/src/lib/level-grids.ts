import type { LevelData } from "../types/level";
import { clampMapSize, ensureLevelCollision } from "./level-collision";
import { ensureLevelFluids } from "./level-fluids";

export function ensureLevelGrids(level: LevelData): LevelData {
  return ensureLevelFluids(ensureLevelCollision(level));
}

export function resizeLevel(level: LevelData, width: number, height: number): LevelData {
  const nextWidth = clampMapSize(width);
  const nextHeight = clampMapSize(height);

  const tiles = level.tiles.filter((tile) => tile.x < nextWidth && tile.y < nextHeight);

  const collision = Array.from({ length: nextHeight }, (_, y) =>
    Array.from({ length: nextWidth }, (_, x) => level.collision[y]?.[x] ?? 0),
  );

  const fluids = Array.from({ length: nextHeight }, (_, y) =>
    Array.from({ length: nextWidth }, (_, x) => level.fluids[y]?.[x] ?? 0),
  );

  return ensureLevelGrids({
    ...level,
    width: nextWidth,
    height: nextHeight,
    tiles,
    collision,
    fluids,
  });
}
