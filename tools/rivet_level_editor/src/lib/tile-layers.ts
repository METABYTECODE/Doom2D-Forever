import {
  FLUID_ACID,
  FLUID_LAVA,
  FLUID_NONE,
  FLUID_WATER,
  GRID_SIZE,
  type PaintFluidOption,
} from "../types/level";
import { paintCollisionCells } from "./level-collision";
import { paintFluidCells } from "./level-fluids";
import { tileFootprintCells } from "./tile-footprint";
import type { TilesetDef } from "../types/tileset";

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

export function applyTilePaintLayers(
  collision: number[][],
  fluids: number[][],
  anchorX: number,
  anchorY: number,
  tilesets: Map<string, TilesetDef>,
  tilesetId: string,
  paintCollision: boolean,
  paintFluid: PaintFluidOption,
  width: number,
  height: number,
  gridSize: number = GRID_SIZE,
): boolean {
  const cells = tileFootprintCells(
    anchorX,
    anchorY,
    tilesets,
    tilesetId,
    width,
    height,
    gridSize,
  );
  let changed = false;
  if (paintCollision) {
    changed = paintCollisionCells(collision, cells, 1, width, height) || changed;
  }
  if (paintFluid !== "none") {
    changed =
      paintFluidCells(fluids, cells, paintFluidToId(paintFluid), width, height) || changed;
  }
  return changed;
}
