import type { LevelObject } from "../types/level";
import { gridLineCells } from "./level-collision";
import { normalizeRect, rectSubCells } from "./grid-rect";
import { objectColliderAabb } from "./object-collider";
import type { ModelData } from "../types/model";
import { worldToSubCell } from "./sub-grid";

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

/** Pixel footprint for a sub-cell span (fill / marquee). */
export function cellSpanFootprint(
  px0: number,
  py0: number,
  px1: number,
  py1: number,
  gridSize: number,
): { x: number; y: number; w: number; h: number } {
  const { x0, y0, x1, y1 } = normalizeRect(px0, py0, px1, py1);
  const gx0 = Math.floor(x0 / gridSize);
  const gy0 = Math.floor(y0 / gridSize);
  const gx1 = Math.floor((x1 - 1) / gridSize);
  const gy1 = Math.floor((y1 - 1) / gridSize);
  return {
    x: gx0 * gridSize,
    y: gy0 * gridSize,
    w: (gx1 - gx0 + 1) * gridSize,
    h: (gy1 - gy0 + 1) * gridSize,
  };
}

/** Sub-grid cells for line-tool preview (matches collision/fluid line paint). */
export function previewCellsForLine(
  x0: number,
  y0: number,
  x1: number,
  y1: number,
  gridSize: number,
  snap: boolean,
): Array<{ x: number; y: number }> {
  const locked = constrainAxisLineEnd(x0, y0, x1, y1, gridSize, snap);
  const c0 = worldToSubCell(locked.x0, locked.y0, gridSize);
  const c1 = worldToSubCell(locked.x1, locked.y1, gridSize);
  return gridLineCells(c0.x, c0.y, c1.x, c1.y);
}

/** Sub-grid cells for fill-tool preview (matches collision/fluid fill paint). */
export function previewCellsForFill(
  px0: number,
  py0: number,
  px1: number,
  py1: number,
  gridSize: number,
  cols: number,
  rows: number,
): Array<{ x: number; y: number }> {
  return rectSubCells(px0, py0, px1, py1, gridSize, cols, rows);
}
