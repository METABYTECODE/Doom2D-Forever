import type { TilesetDef } from "../types/tileset";

export function tilePixelSize(tileset: TilesetDef): { w: number; h: number } {
  return { w: tileset.tile_width, h: tileset.tile_height };
}

export function tileIndexToAtlasCell(tileId: number, columns: number): { col: number; row: number } {
  return {
    col: tileId % columns,
    row: Math.floor(tileId / columns),
  };
}

export function atlasCellToTileIndex(col: number, row: number, columns: number): number {
  return row * columns + col;
}

export function rectsOverlap(
  ax: number,
  ay: number,
  aw: number,
  ah: number,
  bx: number,
  by: number,
  bw: number,
  bh: number,
): boolean {
  return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}
