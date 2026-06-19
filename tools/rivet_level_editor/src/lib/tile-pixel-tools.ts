/** Axis-aligned line: tiles stepped along horizontal or vertical (dominant axis). */
export function pixelAxisLinePositions(
  x0: number,
  y0: number,
  x1: number,
  y1: number,
  step: number,
): Array<{ x: number; y: number }> {
  const dx = x1 - x0;
  const dy = y1 - y0;
  const adx = Math.abs(dx);
  const ady = Math.abs(dy);

  if (adx < 1e-6 && ady < 1e-6) return [{ x: x0, y: y0 }];
  if (step <= 0) return [{ x: x0, y: y0 }];

  if (adx >= ady) {
    const count = Math.max(1, Math.floor(adx / step));
    const sx = dx >= 0 ? 1 : -1;
    return Array.from({ length: count }, (_, i) => ({
      x: x0 + sx * step * i,
      y: y0,
    }));
  }

  const count = Math.max(1, Math.floor(ady / step));
  const sy = dy >= 0 ? 1 : -1;
  return Array.from({ length: count }, (_, i) => ({
    x: x0,
    y: y0 + sy * step * i,
  }));
}

/** Fill a pixel rect with tile anchors on a tw×th grid from snapped top-left. */
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
  const w = maxX - minX;
  const h = maxY - minY;
  if (w < 1e-6 && h < 1e-6) {
    return [{ x: minX, y: minY }];
  }
  const cols = Math.max(1, Math.floor(w / tileWidth));
  const rows = Math.max(1, Math.floor(h / tileHeight));
  const positions: Array<{ x: number; y: number }> = [];
  for (let row = 0; row < rows; row++) {
    for (let col = 0; col < cols; col++) {
      positions.push({ x: minX + col * tileWidth, y: minY + row * tileHeight });
    }
  }
  return positions;
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
