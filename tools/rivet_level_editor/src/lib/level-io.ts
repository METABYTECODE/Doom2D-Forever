import {
  GRID_SIZE,
  LEVEL_FORMAT,
  LEVEL_VERSION,
  type LevelData,
  type LevelObject,
  type PlacedTile,
} from "../types/level";
import { ensureLevelCollision, emptyCollision, paintCollisionBorder } from "./level-collision";

export { ensureLevelCollision, resizeLevel } from "./level-collision";

export function createBlankLevel(width = 80, height = 60): LevelData {
  const level: LevelData = {
    format: LEVEL_FORMAT,
    version: LEVEL_VERSION,
    name: "untitled",
    grid_size: GRID_SIZE,
    width,
    height,
    background: "",
    music: "",
    tiles: [],
    collision: emptyCollision(width, height),
    objects: [],
  };
  return paintCollisionBorder(level);
}

function parsePlacedTile(raw: Record<string, unknown>): PlacedTile {
  return {
    tileset: String(raw.tileset),
    id: Number(raw.id),
    x: Number(raw.x),
    y: Number(raw.y),
  };
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

function convertLegacyLevel(raw: Record<string, unknown>): LevelData {
  const legacyTileSize = Number(raw.tile_size ?? 32);
  const scale = legacyTileSize / GRID_SIZE;
  if (!Number.isInteger(scale) || scale < 1) {
    throw new Error("Legacy tile_size must be a multiple of 8");
  }

  const legacyTiles = raw.tiles as number[][];
  const legacyWidth = Number(raw.width ?? legacyTiles[0]?.length ?? 0);
  const legacyHeight = Number(raw.height ?? legacyTiles.length ?? 0);
  const width = legacyWidth * scale;
  const height = legacyHeight * scale;
  const collision = emptyCollision(width, height);

  for (let y = 0; y < legacyHeight; y++) {
    for (let x = 0; x < legacyWidth; x++) {
      if ((legacyTiles[y]?.[x] ?? 0) !== 1) continue;
      for (let dy = 0; dy < scale; dy++) {
        for (let dx = 0; dx < scale; dx++) {
          collision[y * scale + dy][x * scale + dx] = 1;
        }
      }
    }
  }

  const objects = Array.isArray(raw.objects)
    ? raw.objects.map((item, index) => parseObject(item as Record<string, unknown>, index))
    : [];

  return ensureLevelCollision({
    format: LEVEL_FORMAT,
    version: LEVEL_VERSION,
    name: String(raw.name ?? "level"),
    grid_size: GRID_SIZE,
    width,
    height,
    background: "",
    music: "",
    tiles: [],
    collision,
    objects,
  });
}

export function parseLevelJson(text: string): LevelData {
  const raw = JSON.parse(text) as Record<string, unknown>;
  if (raw.format !== LEVEL_FORMAT) {
    throw new Error(`Unsupported format: ${String(raw.format)}`);
  }

  const version = Number(raw.version);
  if (version === 1) {
    return convertLegacyLevel(raw);
  }
  if (version !== LEVEL_VERSION) {
    throw new Error(`Unsupported version: ${String(raw.version)}`);
  }

  const width = Number(raw.width ?? 0);
  const height = Number(raw.height ?? 0);

  const tiles = Array.isArray(raw.tiles)
    ? raw.tiles.map((item) => parsePlacedTile(item as Record<string, unknown>))
    : [];

  const collision = Array.isArray(raw.collision)
    ? (raw.collision as number[][])
    : emptyCollision(width, height);

  const objects = Array.isArray(raw.objects)
    ? raw.objects.map((item, index) => parseObject(item as Record<string, unknown>, index))
    : [];

  return ensureLevelCollision({
    format: LEVEL_FORMAT,
    version: LEVEL_VERSION,
    name: String(raw.name ?? "level"),
    grid_size: Number(raw.grid_size ?? GRID_SIZE),
    width,
    height,
    background: String(raw.background ?? ""),
    music: String(raw.music ?? ""),
    tiles,
    collision,
    objects,
  });
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
  const normalized = ensureLevelCollision(level);
  const json: Record<string, unknown> = {
    format: LEVEL_FORMAT,
    version: LEVEL_VERSION,
    name: normalized.name,
    grid_size: normalized.grid_size,
    width: normalized.width,
    height: normalized.height,
    tiles: normalized.tiles,
    collision: normalized.collision,
    objects: normalized.objects.map(objectToJson),
  };
  if (normalized.background) json.background = normalized.background;
  if (normalized.music) json.music = normalized.music;
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
