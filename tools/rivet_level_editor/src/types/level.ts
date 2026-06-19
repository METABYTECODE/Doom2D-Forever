export const LEVEL_FORMAT = "rivet-level" as const;
export const LEVEL_VERSION = 2;
export const GRID_SIZE = 8;

export interface LevelObject {
  id: string;
  type: string;
  x: number;
  y: number;
  width: number;
  height: number;
  vel_x: number;
  vel_y: number;
}

export interface TileFrame {
  tileset: string;
  id: number;
}

export interface PlacedTile {
  tileset: string;
  id: number;
  x: number;
  y: number;
  /** Animation frames. Frame #0 is always the primary `id` when editing. */
  frames?: TileFrame[];
  /** Milliseconds per animation frame. */
  frame_ms?: number;
}

export const DEFAULT_FRAME_MS = 120;

export const FLUID_NONE = 0;
export const FLUID_WATER = 1;
export const FLUID_ACID = 2;
export const FLUID_LAVA = 3;

export type FluidId =
  | typeof FLUID_NONE
  | typeof FLUID_WATER
  | typeof FLUID_ACID
  | typeof FLUID_LAVA;

export interface LevelData {
  format: typeof LEVEL_FORMAT;
  version: typeof LEVEL_VERSION;
  name: string;
  grid_size: number;
  width: number;
  height: number;
  resource_pack: string;
  background: string;
  music: string;
  tiles: PlacedTile[];
  collision: number[][];
  /** 0 = none, 1 = water, 2 = acid, 3 = lava */
  fluids: number[][];
  objects: LevelObject[];
}

export type EditorMode = "tiles" | "collision" | "fluids" | "objects";

export type TileTool = "paint" | "erase" | "select";
export type CollisionTool = "paint" | "erase";
export type FluidTool = "water" | "acid" | "lava" | "erase";
export type ObjectTool = "place-player" | "place-block" | "select";

/** Fluid layer applied when painting tiles (tileset dock). */
export type PaintFluidOption = "none" | "water" | "acid" | "lava";
