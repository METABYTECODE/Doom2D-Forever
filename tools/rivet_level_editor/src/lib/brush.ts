import { gridLineCells } from "./level-collision";

export function clampBrushSize(value: number): number {
  if (!Number.isFinite(value)) return 1;
  return Math.min(16, Math.max(1, Math.round(value)));
}

export function brushCells(
  anchorX: number,
  anchorY: number,
  brushSize: number,
  mapWidth: number,
  mapHeight: number,
): Array<{ x: number; y: number }> {
  const size = clampBrushSize(brushSize);
  const cells: Array<{ x: number; y: number }> = [];
  for (let dy = 0; dy < size; dy++) {
    for (let dx = 0; dx < size; dx++) {
      const x = anchorX + dx;
      const y = anchorY + dy;
      if (x < 0 || y < 0 || x >= mapWidth || y >= mapHeight) continue;
      cells.push({ x, y });
    }
  }
  return cells;
}

export function strokeBrushCells(
  strokeFrom: { x: number; y: number } | undefined,
  cellX: number,
  cellY: number,
  brushSize: number,
  mapWidth: number,
  mapHeight: number,
): Array<{ x: number; y: number }> {
  const anchors = strokeFrom
    ? gridLineCells(strokeFrom.x, strokeFrom.y, cellX, cellY)
    : [{ x: cellX, y: cellY }];

  const seen = new Set<string>();
  const cells: Array<{ x: number; y: number }> = [];
  for (const anchor of anchors) {
    for (const cell of brushCells(anchor.x, anchor.y, brushSize, mapWidth, mapHeight)) {
      const key = `${cell.x},${cell.y}`;
      if (seen.has(key)) continue;
      seen.add(key);
      cells.push(cell);
    }
  }
  return cells;
}
