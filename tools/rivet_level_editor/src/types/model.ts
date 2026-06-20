export const MODEL_FORMAT = "rivet-model";
export const MODEL_VERSION = 3;

export const BUILTIN_ANIMATIONS = [
  "IDLE",
  "WALK",
  "RUN",
  "FIRE",
  "FIRE_UP",
  "FIRE_DOWN",
  "PAIN",
  "DIE",
  "JUMP",
  "FALL",
] as const;

export type BuiltinAnimation = (typeof BUILTIN_ANIMATIONS)[number];

export const DEFAULT_FRAME_MS = 120;
export const DEFAULT_FRAME_WIDTH = 64;
export const DEFAULT_FRAME_HEIGHT = 64;
export const DEFAULT_COLUMNS = 1;

/** Entity attachment point in reference frame space (pixels, top-left origin). */
export interface ModelPivot {
  x: number;
  y: number;
}

/** Physics hull in reference frame space — same coords as the sprite frame. */
export interface ModelCollider {
  x: number;
  y: number;
  width: number;
  height: number;
}

export interface ModelFrame {
  id: number;
}

/** One animation clip — atlas path + grid + frame indices. All stored in the model JSON. */
export interface ModelAnimation {
  atlas: string;
  frame_width: number;
  frame_height: number;
  columns: number;
  frames: ModelFrame[];
  frame_ms: number;
  loop: boolean;
}

export interface ModelData {
  format: typeof MODEL_FORMAT;
  version: typeof MODEL_VERSION;
  id: string;
  name: string;
  resource_pack: string;
  pivot: ModelPivot;
  collider: ModelCollider;
  animations: Record<string, ModelAnimation>;
}
