import { DEFAULT_FRAME_MS, type PlacedTile, type TileFrame } from "../types/level";

export function placementFrames(tile: PlacedTile): TileFrame[] {
  if (tile.frames && tile.frames.length > 0) {
    return tile.frames;
  }
  return [{ tileset: tile.tileset, id: tile.id }];
}

export function isAnimatedPlacement(tile: PlacedTile): boolean {
  return placementFrames(tile).length > 1;
}

export function animatedTileFrame(tile: PlacedTile, elapsedMs: number): TileFrame {
  const frames = placementFrames(tile);
  if (frames.length <= 1) {
    return frames[0];
  }
  const frameMs = tile.frame_ms ?? DEFAULT_FRAME_MS;
  const index = Math.floor(elapsedMs / frameMs) % frames.length;
  return frames[index];
}
