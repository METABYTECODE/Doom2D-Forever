import type { PlacedTile } from "../types/level";
import { GRID_SIZE } from "../types/level";
import type { TilesetDef } from "../types/tileset";
import { rectsOverlap, tileCellSpan } from "./tile-math";
import { normalizeRect } from "./grid-rect";

export function placementFootprint(
  placement: PlacedTile,
  tilesets: Map<string, TilesetDef>,
  gridSize: number = GRID_SIZE,
): { x: number; y: number; w: number; h: number } {
  const ts = tilesets.get(placement.tileset);
  const span = ts ? tileCellSpan(ts, gridSize) : { w: 1, h: 1 };
  return { x: placement.x, y: placement.y, w: span.w, h: span.h };
}

export function placementsInRect(
  placements: PlacedTile[],
  tilesets: Map<string, TilesetDef>,
  x0: number,
  y0: number,
  x1: number,
  y1: number,
  gridSize: number = GRID_SIZE,
): number[] {
  const { x0: rx0, y0: ry0, x1: rx1, y1: ry1 } = normalizeRect(x0, y0, x1, y1);
  const rectW = rx1 - rx0 + 1;
  const rectH = ry1 - ry0 + 1;
  const hits: number[] = [];

  placements.forEach((placement, index) => {
    const fp = placementFootprint(placement, tilesets, gridSize);
    if (rectsOverlap(rx0, ry0, rectW, rectH, fp.x, fp.y, fp.w, fp.h)) {
      hits.push(index);
    }
  });

  return hits;
}

export function canMovePlacements(
  placements: PlacedTile[],
  tilesets: Map<string, TilesetDef>,
  indices: number[],
  deltaX: number,
  deltaY: number,
  mapWidth: number,
  mapHeight: number,
  gridSize: number = GRID_SIZE,
): boolean {
  const ignore = new Set(indices);
  for (const index of indices) {
    const placement = placements[index];
    if (!placement) return false;
    const nextX = placement.x + deltaX;
    const nextY = placement.y + deltaY;
    if (
      !canPlaceIgnoring(
        placements,
        tilesets,
        placement.tileset,
        placement.id,
        nextX,
        nextY,
        mapWidth,
        mapHeight,
        ignore,
        gridSize,
      )
    ) {
      return false;
    }
  }
  return true;
}

function canPlaceIgnoring(
  placements: PlacedTile[],
  tilesets: Map<string, TilesetDef>,
  tilesetId: string,
  tileId: number,
  cellX: number,
  cellY: number,
  mapWidth: number,
  mapHeight: number,
  ignore: Set<number>,
  gridSize: number,
): boolean {
  const ts = tilesets.get(tilesetId);
  if (!ts) return false;
  const span = tileCellSpan(ts, gridSize);
  if (cellX < 0 || cellY < 0 || cellX + span.w > mapWidth || cellY + span.h > mapHeight) {
    return false;
  }

  for (let i = 0; i < placements.length; i++) {
    if (ignore.has(i)) continue;
    const other = placements[i];
    const otherTs = tilesets.get(other.tileset);
    const otherSpan = otherTs ? tileCellSpan(otherTs, gridSize) : { w: 1, h: 1 };
    if (
      rectsOverlap(cellX, cellY, span.w, span.h, other.x, other.y, otherSpan.w, otherSpan.h)
    ) {
      return false;
    }
  }

  void tileId;
  return true;
}

export function movePlacements(
  placements: PlacedTile[],
  indices: number[],
  deltaX: number,
  deltaY: number,
): PlacedTile[] {
  const move = new Set(indices);
  return placements.map((tile, i) =>
    move.has(i) ? { ...tile, x: tile.x + deltaX, y: tile.y + deltaY } : tile,
  );
}
