export const LEVEL_FORMAT = "rivet-level" as const;
export const LEVEL_VERSION = 1;

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

export interface LevelData {
  format: typeof LEVEL_FORMAT;
  version: typeof LEVEL_VERSION;
  name: string;
  tile_size: number;
  width: number;
  height: number;
  tiles: number[][];
  objects: LevelObject[];
}

export type EditorTool = "paint" | "erase" | "place-player" | "place-block" | "select";
