import { worldToSubCell } from "./sub-grid";

export function normalizeRect(
  x0: number,
  y0: number,
  x1: number,
  y1: number,
): { x0: number; y0: number; x1: number; y1: number } {
  return {
    x0: Math.min(x0, x1),
    y0: Math.min(y0, y1),
    x1: Math.max(x0, x1),
    y1: Math.max(y0, y1),
  };
}

/** Sub-grid cells inside a pixel rect (for collision/fluids fill). */
export function rectSubCells(
  px0: number,
  py0: number,
  px1: number,
  py1: number,
  gridSize: number,
  cols: number,
  rows: number,
): Array<{ x: number; y: number }> {
  const { x0, y0, x1, y1 } = normalizeRect(px0, py0, px1, py1);
  const c0 = worldToSubCell(x0, y0, gridSize);
  const c1 = worldToSubCell(x1, y1, gridSize);
  const gx0 = Math.min(c0.x, c1.x);
  const gy0 = Math.min(c0.y, c1.y);
  const gx1 = Math.max(c0.x, c1.x);
  const gy1 = Math.max(c0.y, c1.y);
  const cells: Array<{ x: number; y: number }> = [];
  for (let y = gy0; y <= gy1; y++) {
    for (let x = gx0; x <= gx1; x++) {
      if (x < 0 || y < 0 || x >= cols || y >= rows) continue;
      cells.push({ x, y });
    }
  }
  return cells;
}
