import type { TilesetDef } from "../types/tileset";

export interface TileDestRect {
  x: number;
  y: number;
  w: number;
  h: number;
}

/** Draw offset from anchor point (anchor is placement x,y in world px). */
export function tileDrawOffset(tileset: TilesetDef): { ox: number; oy: number } {
  if (tileset.offset_x != null || tileset.offset_y != null) {
    return { ox: tileset.offset_x ?? 0, oy: tileset.offset_y ?? 0 };
  }

  switch (tileset.anchor ?? "top-left") {
    case "bottom-left":
      return { ox: 0, oy: -tileset.tile_height };
    case "center":
      return {
        ox: -tileset.tile_width / 2,
        oy: -tileset.tile_height / 2,
      };
    default:
      return { ox: 0, oy: 0 };
  }
}

export function tileDestRect(
  anchorX: number,
  anchorY: number,
  tileset: TilesetDef,
): TileDestRect {
  const { ox, oy } = tileDrawOffset(tileset);
  return {
    x: anchorX + ox,
    y: anchorY + oy,
    w: tileset.tile_width,
    h: tileset.tile_height,
  };
}

export function pointInRect(px: number, py: number, rect: TileDestRect): boolean {
  return px >= rect.x && py >= rect.y && px < rect.x + rect.w && py < rect.y + rect.h;
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
