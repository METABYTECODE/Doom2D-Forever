import {
  DEFAULT_FRAME_MS,
  DEFAULT_MAP_HEIGHT_PX,
  DEFAULT_MAP_WIDTH_PX,
  GRID_SIZE,
  LEVEL_FORMAT,
  LEVEL_VERSION,
  type LevelData,
  type LevelObject,
  type PlacedTile,
  type TileFrame,
} from "../types/level";
import { DEFAULT_PACK_ID } from "./resource-pack";
import { emptyCollision, paintCollisionBorder } from "./level-collision";
import { emptyFluids } from "./level-fluids";
import { ensureLevelGrids, resizeLevel, setGridSize } from "./level-grids";
import { subGridCols, subGridRows } from "./sub-grid";

export { ensureLevelGrids, resizeLevel, setGridSize };

export function createBlankLevel(
  widthPx = DEFAULT_MAP_WIDTH_PX,
  heightPx = DEFAULT_MAP_HEIGHT_PX,
): LevelData {
  const cols = subGridCols(widthPx, GRID_SIZE);
  const rows = subGridRows(heightPx, GRID_SIZE);
  const level: LevelData = {
    format: LEVEL_FORMAT,
    version: LEVEL_VERSION,
    name: "untitled",
    grid_size: GRID_SIZE,
    width: widthPx,
    height: heightPx,
    resource_pack: DEFAULT_PACK_ID,
    background: "",
    music: "",
    tiles: [],
    collision: emptyCollision(cols, rows),
    fluids: emptyFluids(cols, rows),
    objects: [],
  };
  return paintCollisionBorder(level);
}

function parseTileFrame(raw: Record<string, unknown>): TileFrame {
  return {
    tileset: String(raw.tileset),
    id: Number(raw.id),
  };
}

function parsePlacedTile(raw: Record<string, unknown>): PlacedTile {
  const tile: PlacedTile = {
    tileset: String(raw.tileset),
    id: Number(raw.id),
    x: Number(raw.x),
    y: Number(raw.y),
  };
  if (Array.isArray(raw.frames)) {
    tile.frames = raw.frames.map((frame) => parseTileFrame(frame as Record<string, unknown>));
  }
  if (raw.frame_ms != null) {
    tile.frame_ms = Number(raw.frame_ms);
  }
  return tile;
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

function migrateV2ToV3(
  raw: Record<string, unknown>,
  tiles: PlacedTile[],
  collision: number[][],
  fluids: number[][],
  objects: LevelObject[],
): LevelData {
  const gridSize = Number(raw.grid_size ?? GRID_SIZE);
  const widthPx = Number(raw.width ?? 0) * gridSize;
  const heightPx = Number(raw.height ?? 0) * gridSize;
  const migratedTiles = tiles.map((tile) => ({
    ...tile,
    x: tile.x * gridSize,
    y: tile.y * gridSize,
  }));

  return ensureLevelGrids({
    format: LEVEL_FORMAT,
    version: LEVEL_VERSION,
    name: String(raw.name ?? "level"),
    grid_size: gridSize,
    width: widthPx,
    height: heightPx,
    resource_pack: String(raw.resource_pack ?? DEFAULT_PACK_ID),
    background: String(raw.background ?? ""),
    music: String(raw.music ?? ""),
    tiles: migratedTiles,
    collision,
    fluids,
    objects,
  });
}

function convertLegacyLevel(raw: Record<string, unknown>): LevelData {
  const legacyTileSize = Number(raw.tile_size ?? 32);
  const gridSize = GRID_SIZE;
  const scale = legacyTileSize / gridSize;
  if (!Number.isInteger(scale) || scale < 1) {
    throw new Error("Legacy tile_size must be a multiple of 8");
  }

  const legacyTiles = raw.tiles as number[][];
  const legacyWidth = Number(raw.width ?? legacyTiles[0]?.length ?? 0);
  const legacyHeight = Number(raw.height ?? legacyTiles.length ?? 0);
  const cols = legacyWidth * scale;
  const rows = legacyHeight * scale;
  const widthPx = cols * gridSize;
  const heightPx = rows * gridSize;
  const collision = emptyCollision(cols, rows);

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

  return ensureLevelGrids({
    format: LEVEL_FORMAT,
    version: LEVEL_VERSION,
    name: String(raw.name ?? "level"),
    grid_size: gridSize,
    width: widthPx,
    height: heightPx,
    resource_pack: DEFAULT_PACK_ID,
    background: "",
    music: "",
    tiles: [],
    collision,
    fluids: emptyFluids(cols, rows),
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
  if (version !== 2 && version !== LEVEL_VERSION) {
    throw new Error(`Unsupported version: ${String(raw.version)}`);
  }

  const gridSize = Number(raw.grid_size ?? GRID_SIZE);
  const width = Number(raw.width ?? 0);
  const height = Number(raw.height ?? 0);

  const tiles = Array.isArray(raw.tiles)
    ? raw.tiles.map((item) => parsePlacedTile(item as Record<string, unknown>))
    : [];

  const collision = Array.isArray(raw.collision)
    ? (raw.collision as number[][])
    : emptyCollision(subGridCols(width * (version === 2 ? gridSize : 1), gridSize), subGridRows(height * (version === 2 ? gridSize : 1), gridSize));

  const fluids = Array.isArray(raw.fluids)
    ? (raw.fluids as number[][])
    : emptyFluids(subGridCols(width * (version === 2 ? gridSize : 1), gridSize), subGridRows(height * (version === 2 ? gridSize : 1), gridSize));

  const objects = Array.isArray(raw.objects)
    ? raw.objects.map((item, index) => parseObject(item as Record<string, unknown>, index))
    : [];

  if (version === 2) {
    return migrateV2ToV3(raw, tiles, collision, fluids, objects);
  }

  return ensureLevelGrids({
    format: LEVEL_FORMAT,
    version: LEVEL_VERSION,
    name: String(raw.name ?? "level"),
    grid_size: gridSize,
    width,
    height,
    resource_pack: String(raw.resource_pack ?? DEFAULT_PACK_ID),
    background: String(raw.background ?? ""),
    music: String(raw.music ?? ""),
    tiles,
    collision,
    fluids,
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

function placedTileToJson(tile: PlacedTile): Record<string, unknown> {
  const json: Record<string, unknown> = {
    tileset: tile.tileset,
    id: tile.id,
    x: tile.x,
    y: tile.y,
  };
  if (tile.frames && tile.frames.length > 1) {
    json.frames = tile.frames;
    json.frame_ms = tile.frame_ms ?? DEFAULT_FRAME_MS;
  }
  return json;
}

function fluidsGridHasContent(fluids: number[][]): boolean {
  return fluids.some((row) => row.some((cell) => cell !== 0));
}

export function serializeLevel(level: LevelData): string {
  const normalized = ensureLevelGrids(level);
  const json: Record<string, unknown> = {
    format: LEVEL_FORMAT,
    version: LEVEL_VERSION,
    name: normalized.name,
    grid_size: normalized.grid_size,
    width: normalized.width,
    height: normalized.height,
    tiles: normalized.tiles.map(placedTileToJson),
    collision: normalized.collision,
    objects: normalized.objects.map(objectToJson),
  };
  if (fluidsGridHasContent(normalized.fluids)) {
    json.fluids = normalized.fluids;
  }
  if (normalized.background) json.background = normalized.background;
  if (normalized.music) json.music = normalized.music;
  if (normalized.resource_pack !== DEFAULT_PACK_ID) {
    json.resource_pack = normalized.resource_pack;
  }
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
