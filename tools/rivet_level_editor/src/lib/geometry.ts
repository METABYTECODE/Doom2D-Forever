import type { LevelObject } from "../types/level";

export function hitObject(objects: LevelObject[], x: number, y: number): number {
  for (let i = objects.length - 1; i >= 0; i--) {
    const o = objects[i];
    if (x >= o.x && x <= o.x + o.width && y >= o.y && y <= o.y + o.height) {
      return i;
    }
  }
  return -1;
}

export function snapCoord(value: number, tileSize: number): number {
  return Math.round(value / tileSize) * tileSize;
}

export function snapPoint(x: number, y: number, tileSize: number): { x: number; y: number } {
  return { x: snapCoord(x, tileSize), y: snapCoord(y, tileSize) };
}

export function tileAt(worldX: number, worldY: number, tileSize: number) {
  return {
    x: Math.floor(worldX / tileSize),
    y: Math.floor(worldY / tileSize),
  };
}
