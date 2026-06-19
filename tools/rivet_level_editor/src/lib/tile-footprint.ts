import type { TilesetDef } from "../types/tileset";
import { GRID_SIZE } from "../types/level";
import { tileCellSpan } from "./tile-math";

export function tileFootprintCells(
  anchorX: number,
  anchorY: number,
  tilesets: Map<string, TilesetDef>,
  tilesetId: string,
  mapWidth: number,
  mapHeight: number,
  gridSize: number = GRID_SIZE,
): Array<{ x: number; y: number }> {
  const tileset = tilesets.get(tilesetId);
  const span = tileset ? tileCellSpan(tileset, gridSize) : { w: 1, h: 1 };
  const cells: Array<{ x: number; y: number }> = [];
  for (let dy = 0; dy < span.h; dy++) {
    for (let dx = 0; dx < span.w; dx++) {
      const x = anchorX + dx;
      const y = anchorY + dy;
      if (x < 0 || y < 0 || x >= mapWidth || y >= mapHeight) continue;
      cells.push({ x, y });
    }
  }
  return cells;
}

export function tileFootprintSpan(
  tilesets: Map<string, TilesetDef>,
  tilesetId: string,
  gridSize: number = GRID_SIZE,
): { w: number; h: number } {
  const tileset = tilesets.get(tilesetId);
  return tileset ? tileCellSpan(tileset, gridSize) : { w: 1, h: 1 };
}
