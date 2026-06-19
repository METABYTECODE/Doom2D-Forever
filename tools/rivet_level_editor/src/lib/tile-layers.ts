import {
  FLUID_ACID,
  FLUID_LAVA,
  FLUID_NONE,
  FLUID_WATER,
  type PaintFluidOption,
} from "../types/level";
import type { TilesetDef } from "../types/tileset";
import { paintCollisionCells } from "./level-collision";
import { paintFluidCells } from "./level-fluids";
import { tileDestRect } from "./tile-render";

export function paintFluidToId(option: PaintFluidOption): number {
  switch (option) {
    case "water":
      return FLUID_WATER;
    case "acid":
      return FLUID_ACID;
    case "lava":
      return FLUID_LAVA;
    default:
      return FLUID_NONE;
  }
}

function footprintSubCells(
  anchorX: number,
  anchorY: number,
  tileset: TilesetDef | null,
  gridSize: number,
): Array<{ x: number; y: number }> {
  const dest = tileset
    ? tileDestRect(anchorX, anchorY, tileset)
    : { x: anchorX, y: anchorY, w: gridSize, h: gridSize };
  const x0 = Math.floor(dest.x / gridSize);
  const y0 = Math.floor(dest.y / gridSize);
  const x1 = Math.floor((dest.x + dest.w - 1) / gridSize);
  const y1 = Math.floor((dest.y + dest.h - 1) / gridSize);
  const cells: Array<{ x: number; y: number }> = [];
  for (let y = y0; y <= y1; y++) {
    for (let x = x0; x <= x1; x++) {
      cells.push({ x, y });
    }
  }
  return cells;
}

/** Stamp collision/fluid sub-cells covered by the tile sprite footprint. */
export function applyTilePaintLayers(
  collision: number[][],
  fluids: number[][],
  anchorX: number,
  anchorY: number,
  tileset: TilesetDef | null,
  paintCollision: boolean,
  paintFluid: PaintFluidOption,
  cols: number,
  rows: number,
  gridSize: number,
): boolean {
  const cells = footprintSubCells(anchorX, anchorY, tileset, gridSize);
  let changed = false;
  if (paintCollision) {
    changed = paintCollisionCells(collision, cells, 1, cols, rows) || changed;
  }
  if (paintFluid !== "none") {
    changed = paintFluidCells(fluids, cells, paintFluidToId(paintFluid), cols, rows) || changed;
  }
  return changed;
}
