import type { PlacedTile } from "../types/level";
import type { TilesetDef } from "../types/tileset";
import { pointInRect, tileDestRect } from "./tile-render";

const ANCHOR_EPS = 0.5;

/** Hit-test by visible sprite bounds (topmost wins). */
export function findPlacementAtWorld(
  placements: PlacedTile[],
  tilesets: Map<string, TilesetDef>,
  worldX: number,
  worldY: number,
): number {
  for (let i = placements.length - 1; i >= 0; i--) {
    const placement = placements[i];
    const tileset = tilesets.get(placement.tileset);
    if (!tileset) continue;
    const rect = tileDestRect(placement.x, placement.y, tileset);
    if (pointInRect(worldX, worldY, rect)) return i;
  }
  return -1;
}

/** Replace-on-paint: exact anchor match. */
export function findPlacementAtAnchor(
  placements: PlacedTile[],
  anchorX: number,
  anchorY: number,
): number {
  for (let i = placements.length - 1; i >= 0; i--) {
    const p = placements[i];
    if (Math.abs(p.x - anchorX) < ANCHOR_EPS && Math.abs(p.y - anchorY) < ANCHOR_EPS) {
      return i;
    }
  }
  return -1;
}

export function placeTilesAtPositions(
  placements: PlacedTile[],
  positions: Array<{ x: number; y: number }>,
  tilesetId: string,
  tileId: number,
  mapWidthPx: number,
  mapHeightPx: number,
): PlacedTile[] {
  const seen = new Set<string>();
  const newOnes: PlacedTile[] = [];
  for (const { x, y } of positions) {
    if (x < 0 || y < 0 || x >= mapWidthPx || y >= mapHeightPx) continue;
    const key = `${Math.round(x)}:${Math.round(y)}`;
    if (seen.has(key)) continue;
    seen.add(key);
    newOnes.push({ tileset: tilesetId, id: tileId, x, y });
  }
  if (newOnes.length === 0) return placements;

  const replaceAnchors = new Set(newOnes.map((t) => `${Math.round(t.x)}:${Math.round(t.y)}`));
  const kept = placements.filter(
    (t) => !replaceAnchors.has(`${Math.round(t.x)}:${Math.round(t.y)}`),
  );
  void tileId;
  return [...kept, ...newOnes];
}
