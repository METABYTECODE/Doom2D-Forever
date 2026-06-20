import { gridLineCells } from "./level-collision";
import { worldToSubCell } from "./sub-grid";

/** Axis-aligned line: tile anchors along dominant axis (inclusive, matches gridLineCells). */
export function pixelAxisLinePositions(
  x0: number,
  y0: number,
  x1: number,
  y1: number,
  step: number,
): Array<{ x: number; y: number }> {
  if (step <= 0) return [{ x: x0, y: y0 }];
  const c0 = worldToSubCell(x0, y0, step);
  const c1 = worldToSubCell(x1, y1, step);
  const cells = gridLineCells(c0.x, c0.y, c1.x, c1.y);
  return cells.map((cell) => ({ x: cell.x * step, y: cell.y * step }));
}

/** Fill a pixel rect with tile anchors — inclusive cell span (matches rectSubCells). */
export function pixelFillPositions(
  x0: number,
  y0: number,
  x1: number,
  y1: number,
  tileWidth: number,
  tileHeight: number,
): Array<{ x: number; y: number }> {
  const minX = Math.min(x0, x1);
  const minY = Math.min(y0, y1);
  const maxX = Math.max(x0, x1);
  const maxY = Math.max(y0, y1);
  const gx0 = Math.floor(minX / tileWidth);
  const gy0 = Math.floor(minY / tileHeight);
  const gx1 = Math.floor((maxX - 1) / tileWidth);
  const gy1 = Math.floor((maxY - 1) / tileHeight);
  const positions: Array<{ x: number; y: number }> = [];
  for (let gy = gy0; gy <= gy1; gy++) {
    for (let gx = gx0; gx <= gx1; gx++) {
      positions.push({ x: gx * tileWidth, y: gy * tileHeight });
    }
  }
  return positions.length > 0 ? positions : [{ x: minX, y: minY }];
}

/** Brush stroke interpolation along a path (may be diagonal). */
export function pixelLinePositions(
  x0: number,
  y0: number,
  x1: number,
  y1: number,
  step: number,
): Array<{ x: number; y: number }> {
  const dx = x1 - x0;
  const dy = y1 - y0;
  const dist = Math.hypot(dx, dy);
  if (dist < 1e-6 || step <= 0) return [{ x: x0, y: y0 }];

  const count = Math.max(1, Math.floor(dist / step));
  const ux = dx / dist;
  const uy = dy / dist;
  const positions: Array<{ x: number; y: number }> = [];
  for (let i = 0; i < count; i++) {
    positions.push({ x: x0 + ux * step * i, y: y0 + uy * step * i });
  }
  return positions;
}
