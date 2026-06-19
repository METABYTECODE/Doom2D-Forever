import type { ModelAnimation } from "../types/model";
import {
  DEFAULT_COLUMNS,
  DEFAULT_FRAME_HEIGHT,
  DEFAULT_FRAME_MS,
  DEFAULT_FRAME_WIDTH,
} from "../types/model";
import type { TilesetDef } from "../types/tileset";

export function defaultAnimation(atlas = ""): ModelAnimation {
  return {
    atlas,
    frame_width: DEFAULT_FRAME_WIDTH,
    frame_height: DEFAULT_FRAME_HEIGHT,
    columns: DEFAULT_COLUMNS,
    frames: [{ id: 0 }],
    frame_ms: DEFAULT_FRAME_MS,
    loop: true,
  };
}

export function animationToTileset(anim: ModelAnimation, textureUrl = ""): TilesetDef {
  return {
    id: anim.atlas,
    name: anim.atlas,
    textureUrl,
    tile_width: anim.frame_width,
    tile_height: anim.frame_height,
    columns: Math.max(1, anim.columns),
  };
}

export function suggestColumns(imageWidth: number, frameWidth: number): number {
  const cell = Math.max(1, frameWidth);
  return Math.max(1, Math.floor(imageWidth / cell));
}

export function atlasRows(imageHeight: number, frameHeight: number): number {
  const cell = Math.max(1, frameHeight);
  return Math.max(1, Math.ceil(imageHeight / cell));
}
