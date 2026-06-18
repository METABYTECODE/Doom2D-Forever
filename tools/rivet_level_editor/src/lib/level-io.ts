import {
  LEVEL_FORMAT,
  LEVEL_VERSION,
  type LevelData,
  type LevelObject,
} from "../types/level";
import { ensureLevelTiles } from "./level-tiles";

export { ensureLevelTiles, resizeLevel as resizeTiles } from "./level-tiles";

export function createBlankLevel(width = 20, height = 15, tileSize = 32): LevelData {
  const tiles = Array.from({ length: height }, () => Array<number>(width).fill(0));
  for (let x = 0; x < width; x++) {
    tiles[0][x] = 1;
    tiles[height - 1][x] = 1;
  }
  for (let y = 1; y < height - 1; y++) {
    tiles[y][0] = 1;
    tiles[y][width - 1] = 1;
  }

  return ensureLevelTiles({
    format: LEVEL_FORMAT,
    version: LEVEL_VERSION,
    name: "untitled",
    tile_size: tileSize,
    width,
    height,
    tiles,
    objects: [],
  });
}

export function parseLevelJson(text: string): LevelData {
  const raw = JSON.parse(text) as Record<string, unknown>;
  if (raw.format !== LEVEL_FORMAT) {
    throw new Error(`Unsupported format: ${String(raw.format)}`);
  }
  if (raw.version !== LEVEL_VERSION) {
    throw new Error(`Unsupported version: ${String(raw.version)}`);
  }

  const tiles = raw.tiles as number[][];
  const width = Number(raw.width ?? tiles[0]?.length ?? 0);
  const height = Number(raw.height ?? tiles.length ?? 0);

  const objects = Array.isArray(raw.objects)
    ? raw.objects.map((item, index) => parseObject(item as Record<string, unknown>, index))
    : [];

  return ensureLevelTiles({
    format: LEVEL_FORMAT,
    version: LEVEL_VERSION,
    name: String(raw.name ?? "level"),
    tile_size: Number(raw.tile_size ?? 32),
    width,
    height,
    tiles,
    objects,
  });
}

function parseObject(raw: Record<string, unknown>, index: number): LevelObject {
  return {
    id: String(raw.id ?? `object_${index}`),
    type: String(raw.type ?? "block"),
    x: Number(raw.x ?? 0),
    y: Number(raw.y ?? 0),
    width: Number(raw.width ?? 48),
    height: Number(raw.height ?? 48),
    vel_x: Number(raw.vx ?? raw.vel_x ?? 0),
    vel_y: Number(raw.vy ?? raw.vel_y ?? 0),
  };
}

function objectToJson(object: LevelObject): Record<string, unknown> {
  const json: Record<string, unknown> = {
    type: object.type,
    x: object.x,
    y: object.y,
  };
  if (object.id) json.id = object.id;
  if (object.width !== 48) json.width = object.width;
  if (object.height !== 48) json.height = object.height;
  if (object.vel_x !== 0) json.vx = object.vel_x;
  if (object.vel_y !== 0) json.vy = object.vel_y;
  return json;
}

export function serializeLevel(level: LevelData): string {
  const normalized = ensureLevelTiles(level);
  const json = {
    format: LEVEL_FORMAT,
    version: LEVEL_VERSION,
    name: normalized.name,
    tile_size: normalized.tile_size,
    width: normalized.width,
    height: normalized.height,
    tiles: normalized.tiles,
    objects: normalized.objects.map(objectToJson),
  };
  return `${JSON.stringify(json, null, 2)}\n`;
}

export function downloadLevel(level: LevelData, filename: string): void {
  const blob = new Blob([serializeLevel(level)], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement("a");
  anchor.href = url;
  anchor.download = filename.endsWith(".json") ? filename : `${filename}.json`;
  anchor.click();
  URL.revokeObjectURL(url);
}
