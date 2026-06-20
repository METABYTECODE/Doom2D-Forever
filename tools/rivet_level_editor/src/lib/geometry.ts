import type { LevelObject } from "../types/level";
import { objectColliderAabb } from "./object-collider";
import type { ModelData } from "../types/model";

export function hitObject(
  objects: LevelObject[],
  x: number,
  y: number,
  models: ReadonlyMap<string, ModelData> = new Map(),
): number {
  for (let i = objects.length - 1; i >= 0; i--) {
    const o = objects[i];
    const box = objectColliderAabb(o, models);
    if (x >= box.x && x <= box.x + box.width && y >= box.y && y <= box.y + box.height) {
      return i;
    }
  }
  return -1;
}

/** Snap to the grid cell containing this coordinate (top-left of cell). */
export function snapCoord(value: number, gridSize: number): number {
  return Math.floor(value / gridSize) * gridSize;
}

export function snapPoint(x: number, y: number, tileSize: number): { x: number; y: number } {
  return { x: snapCoord(x, tileSize), y: snapCoord(y, tileSize) };
}

/** Snap world coords to grid when enabled (all tile tools use this). */
export function snapWorldPoint(
  x: number,
  y: number,
  gridSize: number,
  snap: boolean,
): { x: number; y: number } {
  return snap ? snapPoint(x, y, gridSize) : { x, y };
}

/**
 * Line tool: lock to H or V by raw cursor delta (not snapped end),
 * then snap only along the active axis. Stops diagonal snap jumps.
 */
export function constrainAxisLineEnd(
  x0: number,
  y0: number,
  x1: number,
  y1: number,
  gridSize: number,
  snap: boolean,
): { x0: number; y0: number; x1: number; y1: number } {
  const start = snapWorldPoint(x0, y0, gridSize, snap);
  const dx = x1 - start.x;
  const dy = y1 - start.y;

  if (Math.abs(dx) >= Math.abs(dy)) {
    const endX = snap ? snapCoord(x1, gridSize) : x1;
    return { x0: start.x, y0: start.y, x1: endX, y1: start.y };
  }

  const endY = snap ? snapCoord(y1, gridSize) : y1;
  return { x0: start.x, y0: start.y, x1: start.x, y1: endY };
}

export function linePreviewRect(
  x0: number,
  y0: number,
  x1: number,
  y1: number,
  gridSize: number,
): { x: number; y: number; w: number; h: number } {
  const minX = Math.min(x0, x1);
  const minY = Math.min(y0, y1);
  const maxX = Math.max(x0, x1);
  const maxY = Math.max(y0, y1);
  const w = maxX - minX + gridSize;
  const h = maxY - minY + gridSize;
  return { x: minX, y: minY, w, h };
}
