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

export function rectCells(
  x0: number,
  y0: number,
  x1: number,
  y1: number,
  mapWidth: number,
  mapHeight: number,
): Array<{ x: number; y: number }> {
  const { x0: minX, y0: minY, x1: maxX, y1: maxY } = normalizeRect(x0, y0, x1, y1);
  const cells: Array<{ x: number; y: number }> = [];
  for (let y = minY; y <= maxY; y++) {
    for (let x = minX; x <= maxX; x++) {
      if (x < 0 || y < 0 || x >= mapWidth || y >= mapHeight) continue;
      cells.push({ x, y });
    }
  }
  return cells;
}
