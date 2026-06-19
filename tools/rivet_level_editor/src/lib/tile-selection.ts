import type { PlacedTile } from "../types/level";
import type { TilesetDef } from "../types/tileset";
import { normalizeRect } from "./grid-rect";
import { rectsOverlap, tileDestRect } from "./tile-render";

export function placementsInRect(
  placements: PlacedTile[],
  tilesets: Map<string, TilesetDef>,
  px0: number,
  py0: number,
  px1: number,
  py1: number,
): number[] {
  const { x0, y0, x1, y1 } = normalizeRect(px0, py0, px1, py1);
  const selW = x1 - x0;
  const selH = y1 - y0;
  const hits: number[] = [];
  placements.forEach((placement, index) => {
    const tileset = tilesets.get(placement.tileset);
    if (!tileset) return;
    const dest = tileDestRect(placement.x, placement.y, tileset);
    if (rectsOverlap(x0, y0, selW, selH, dest.x, dest.y, dest.w, dest.h)) {
      hits.push(index);
    }
  });
  return hits;
}

export function canMovePlacements(
  placements: PlacedTile[],
  indices: number[],
  deltaX: number,
  deltaY: number,
  mapWidthPx: number,
  mapHeightPx: number,
): boolean {
  for (const index of indices) {
    const placement = placements[index];
    if (!placement) return false;
    const nextX = placement.x + deltaX;
    const nextY = placement.y + deltaY;
    if (nextX < 0 || nextY < 0 || nextX >= mapWidthPx || nextY >= mapHeightPx) return false;
  }
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
