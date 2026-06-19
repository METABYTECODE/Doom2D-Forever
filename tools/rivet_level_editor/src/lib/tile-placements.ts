import type { PlacedTile } from "../types/level";
import { GRID_SIZE } from "../types/level";
import type { TilesetDef } from "../types/tileset";
import { rectsOverlap, tileCellSpan } from "./tile-math";

export interface PlacementOccupancy {
  placementIndex: number;
}

export function buildOccupancy(
  placements: PlacedTile[],
  tilesets: Map<string, TilesetDef>,
  gridSize: number = GRID_SIZE,
): (PlacementOccupancy | null)[][] {
  const width = placements.reduce((max, tile) => {
    const ts = tilesets.get(tile.tileset);
    const span = ts ? tileCellSpan(ts, gridSize) : { w: 1, h: 1 };
    return Math.max(max, tile.x + span.w);
  }, 0);
  const height = placements.reduce((max, tile) => {
    const ts = tilesets.get(tile.tileset);
    const span = ts ? tileCellSpan(ts, gridSize) : { w: 1, h: 1 };
    return Math.max(max, tile.y + span.h);
  }, 0);

  const grid: (PlacementOccupancy | null)[][] = Array.from({ length: height }, () =>
    Array<PlacementOccupancy | null>(width).fill(null),
  );

  placements.forEach((placement, placementIndex) => {
    const ts = tilesets.get(placement.tileset);
    const span = ts ? tileCellSpan(ts, gridSize) : { w: 1, h: 1 };
    for (let dy = 0; dy < span.h; dy++) {
      for (let dx = 0; dx < span.w; dx++) {
        const y = placement.y + dy;
        const x = placement.x + dx;
        if (!grid[y]) grid[y] = [];
        grid[y][x] = { placementIndex };
      }
    }
  });

  return grid;
}

export function findPlacementAt(
  placements: PlacedTile[],
  tilesets: Map<string, TilesetDef>,
  cellX: number,
  cellY: number,
  gridSize: number = GRID_SIZE,
): number {
  for (let i = placements.length - 1; i >= 0; i--) {
    const placement = placements[i];
    const ts = tilesets.get(placement.tileset);
    const span = ts ? tileCellSpan(ts, gridSize) : { w: 1, h: 1 };
    if (
      cellX >= placement.x &&
      cellY >= placement.y &&
      cellX < placement.x + span.w &&
      cellY < placement.y + span.h
    ) {
      return i;
    }
  }
  return -1;
}

export function canPlaceTile(
  placements: PlacedTile[],
  tilesets: Map<string, TilesetDef>,
  tilesetId: string,
  tileId: number,
  cellX: number,
  cellY: number,
  mapWidth: number,
  mapHeight: number,
  ignoreIndex = -1,
  gridSize: number = GRID_SIZE,
): boolean {
  const ts = tilesets.get(tilesetId);
  if (!ts) return false;
  const span = tileCellSpan(ts, gridSize);
  if (cellX < 0 || cellY < 0 || cellX + span.w > mapWidth || cellY + span.h > mapHeight) {
    return false;
  }

  for (let i = 0; i < placements.length; i++) {
    if (i === ignoreIndex) continue;
    const other = placements[i];
    const otherTs = tilesets.get(other.tileset);
    const otherSpan = otherTs ? tileCellSpan(otherTs, gridSize) : { w: 1, h: 1 };
    if (
      rectsOverlap(
        cellX,
        cellY,
        span.w,
        span.h,
        other.x,
        other.y,
        otherSpan.w,
        otherSpan.h,
      )
    ) {
      return false;
    }
  }

  void tileId;
  return true;
}
