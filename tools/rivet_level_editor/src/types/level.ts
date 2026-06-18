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

export interface LevelData {
  format: typeof LEVEL_FORMAT;
  version: typeof LEVEL_VERSION;
  name: string;
  grid_size: number;
  width: number;
  height: number;
  background: string;
  music: string;
  tiles: PlacedTile[];
  collision: number[][];
  objects: LevelObject[];
}

export type EditorMode = "tiles" | "collision" | "objects";

export type TileTool = "paint" | "erase" | "select";
export type CollisionTool = "paint" | "erase";
export type ObjectTool = "place-player" | "place-block" | "select";
