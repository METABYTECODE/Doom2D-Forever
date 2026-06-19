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
  const gx0 = Math.floor(x0 / gridSize);
  const gy0 = Math.floor(y0 / gridSize);
  const gx1 = Math.floor((x1 - 1) / gridSize);
  const gy1 = Math.floor((y1 - 1) / gridSize);
  const cells: Array<{ x: number; y: number }> = [];
  for (let y = gy0; y <= gy1; y++) {
    for (let x = gx0; x <= gx1; x++) {
      if (x < 0 || y < 0 || x >= cols || y >= rows) continue;
      cells.push({ x, y });
    }
  }
  return cells;
}
