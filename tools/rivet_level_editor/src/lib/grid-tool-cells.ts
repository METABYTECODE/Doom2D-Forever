import type { EditorMode, LevelData } from "../types/level";
import { FLUID_NONE } from "../types/level";
import { rectSubCells } from "./grid-rect";
import { gridLineCells } from "./level-collision";
import { worldToSubCell } from "./sub-grid";

export type GridCell = { x: number; y: number };

export function cellKey(x: number, y: number): string {
  return `${x},${y}`;
}

export function parseCellKey(key: string): GridCell | null {
  const [xs, ys] = key.split(",");
  const x = Number(xs);
  const y = Number(ys);
  if (!Number.isFinite(x) || !Number.isFinite(y)) return null;
  return { x, y };
}

/** Sub-grid cells inside a world pixel rect (inclusive). */
export function cellsInWorldRect(
  x0: number,
  y0: number,
  x1: number,
  y1: number,
  gridSize: number,
  cols: number,
  rows: number,
): GridCell[] {
  return rectSubCells(x0, y0, x1, y1, gridSize, cols, rows);
}

export function gridCellHasContent(
  level: LevelData,
  mode: EditorMode,
  cellX: number,
  cellY: number,
): boolean {
  if (cellX < 0 || cellY < 0) return false;
  if (mode === "collision") {
    return (level.collision[cellY]?.[cellX] ?? 0) !== 0;
  }
  if (mode === "fluids") {
    return (level.fluids[cellY]?.[cellX] ?? 0) !== FLUID_NONE;
  }
  return false;
}

/** Sub-grid cells along a world line (Bresenham). */
export function cellsOnWorldLine(
  x0: number,
  y0: number,
  x1: number,
  y1: number,
  gridSize: number,
): GridCell[] {
  const c0 = worldToSubCell(x0, y0, gridSize);
  const c1 = worldToSubCell(x1, y1, gridSize);
  return gridLineCells(c0.x, c0.y, c1.x, c1.y);
}

/** Sub-grid cells for a brush stroke between two world points. */
export function cellsOnWorldStroke(
  x0: number,
  y0: number,
  x1: number,
  y1: number,
  gridSize: number,
): GridCell[] {
  return cellsOnWorldLine(x0, y0, x1, y1, gridSize);
}

/** Move grid values for selected cells by a cell delta; returns null if blocked or invalid. */
export function translateGridCellValues(
  grid: number[][],
  cellKeys: Iterable<string>,
  deltaX: number,
  deltaY: number,
  cols: number,
  rows: number,
  emptyValue: number,
): { grid: number[][]; movedKeys: Set<string> } | null {
  if (deltaX === 0 && deltaY === 0) return null;

  const keys = [...cellKeys];
  const sources = new Set(keys);
  const cells = keys
    .map((key) => parseCellKey(key))
    .filter((cell): cell is GridCell => cell !== null);

  const payloads: Array<{ x: number; y: number; v: number }> = [];
  for (const { x, y } of cells) {
    if (x < 0 || y < 0 || x >= cols || y >= rows) continue;
    const v = grid[y][x];
    if (v !== emptyValue) payloads.push({ x, y, v });
  }
  if (payloads.length === 0) return null;

  const next = grid.map((row) => [...row]);
  for (const { x, y } of cells) {
    if (x >= 0 && y >= 0 && x < cols && y < rows) next[y][x] = emptyValue;
  }

  for (const { x, y, v } of payloads) {
    const tx = x + deltaX;
    const ty = y + deltaY;
    if (tx < 0 || ty < 0 || tx >= cols || ty >= rows) return null;
    const tkey = cellKey(tx, ty);
    if (!sources.has(tkey) && next[ty][tx] !== emptyValue) return null;
  }

  for (const { x, y, v } of payloads) {
    next[y + deltaY][x + deltaX] = v;
  }

  const movedKeys = new Set<string>();
  for (const { x, y } of payloads) {
    const tx = x + deltaX;
    const ty = y + deltaY;
    if (tx >= 0 && ty >= 0 && tx < cols && ty < rows) {
      movedKeys.add(cellKey(tx, ty));
    }
  }

  return { grid: next, movedKeys };
}
