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
  type TileLayer,
} from "../types/level";
import { DEFAULT_PACK_ID } from "./resource-pack";
import { emptyCollision, paintCollisionBorder } from "./level-collision";
import { emptyFluids } from "./level-fluids";
import { DEFAULT_TILE_LAYER_ID, defaultTileLayers, ensureTileLayers, ensureStandardTileLayers } from "./level-tile-layers";
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
    tile_layers: defaultTileLayers(),
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

function parseTileLayer(raw: Record<string, unknown>): TileLayer {
  const tiles = Array.isArray(raw.tiles)
    ? raw.tiles.map((item) => parsePlacedTile(item as Record<string, unknown>))
    : [];
  return {
    id: String(raw.id ?? "main"),
    z: Number(raw.z ?? 0),
    tiles,
  };
}

function parseObject(raw: Record<string, unknown>, index: number): LevelObject {
  const type = String(raw.type ?? "block");
  const object: LevelObject = {
    id: String(raw.id ?? `object_${index}`),
    type,
    x: Number(raw.x ?? 0),
    y: Number(raw.y ?? 0),
    vel_x: Number(raw.vx ?? raw.vel_x ?? 0),
    vel_y: Number(raw.vy ?? raw.vel_y ?? 0),
  };
  if (type === "player") {
    const modelId = String(raw.model ?? "player");
    if (modelId !== "player") {
      object.model = modelId;
    }
  } else if (raw.model != null) {
    object.model = String(raw.model);
  }
  if (type !== "player" && !object.model) {
    object.width = Number(raw.width ?? 48);
    object.height = Number(raw.height ?? 48);
  }
  if (raw.z != null) {
    object.z = Number(raw.z);
  }
  return object;
}

function parseTileLayers(raw: Record<string, unknown>, version: number): TileLayer[] {
  if (version >= 4 && Array.isArray(raw.tile_layers)) {
    return ensureStandardTileLayers({
      format: LEVEL_FORMAT,
      version: LEVEL_VERSION,
      name: "",
      grid_size: GRID_SIZE,
      width: 0,
      height: 0,
      resource_pack: DEFAULT_PACK_ID,
      background: "",
      music: "",
      tile_layers: raw.tile_layers.map((layer) => parseTileLayer(layer as Record<string, unknown>)),
      collision: [],
      fluids: [],
      objects: [],
    }).tile_layers;
  }
  if (Array.isArray(raw.tiles)) {
    return ensureStandardTileLayers({
      format: LEVEL_FORMAT,
      version: LEVEL_VERSION,
      name: "",
      grid_size: GRID_SIZE,
      width: 0,
      height: 0,
      resource_pack: DEFAULT_PACK_ID,
      background: "",
      music: "",
      tile_layers: [
        {
          id: DEFAULT_TILE_LAYER_ID,
          z: 0,
          tiles: raw.tiles.map((item) => parsePlacedTile(item as Record<string, unknown>)),
        },
      ],
      collision: [],
      fluids: [],
      objects: [],
    }).tile_layers;
  }
  return defaultTileLayers();
}

export function parseLevelJson(text: string): LevelData {
  const raw = JSON.parse(text) as Record<string, unknown>;
  if (raw.format !== LEVEL_FORMAT) {
    throw new Error(`Unsupported format: ${String(raw.format)}`);
  }

  const version = Number(raw.version);
  if (version < 3 || version > LEVEL_VERSION) {
    throw new Error(
      `Unsupported rivet-level version ${String(raw.version)} (expected v3–v${LEVEL_VERSION}). Resave the map in the level editor.`,
    );
  }

  const gridSize = Number(raw.grid_size ?? GRID_SIZE);
  const width = Number(raw.width ?? 0);
  const height = Number(raw.height ?? 0);

  const collision = Array.isArray(raw.collision)
    ? (raw.collision as number[][])
    : emptyCollision(subGridCols(width, gridSize), subGridRows(height, gridSize));

  const fluids = Array.isArray(raw.fluids)
    ? (raw.fluids as number[][])
    : emptyFluids(subGridCols(width, gridSize), subGridRows(height, gridSize));

  const objects = Array.isArray(raw.objects)
    ? raw.objects.map((item, index) => parseObject(item as Record<string, unknown>, index))
    : [];

  return ensureTileLayers(
    ensureLevelGrids({
      format: LEVEL_FORMAT,
      version: LEVEL_VERSION,
      name: String(raw.name ?? "level"),
      grid_size: gridSize,
      width,
      height,
      resource_pack: String(raw.resource_pack ?? DEFAULT_PACK_ID),
      background: String(raw.background ?? ""),
      music: String(raw.music ?? ""),
      tile_layers: parseTileLayers(raw, version),
      collision,
      fluids,
      objects,
    }),
  );
}

function objectToJson(object: LevelObject): Record<string, unknown> {
  const json: Record<string, unknown> = {
    type: object.type,
    x: object.x,
    y: object.y,
  };
  if (object.id) json.id = object.id;
  if (object.type === "player") {
    if (object.model && object.model !== "player") {
      json.model = object.model;
    }
  } else {
    if (object.model) json.model = object.model;
    if (object.width != null) json.width = object.width;
    if (object.height != null) json.height = object.height;
  }
  if (object.vel_x !== 0) json.vx = object.vel_x;
  if (object.vel_y !== 0) json.vy = object.vel_y;
  if (object.z != null && object.z !== 0) json.z = object.z;
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

function tileLayerToJson(layer: TileLayer): Record<string, unknown> {
  return {
    id: layer.id,
    z: layer.z,
    tiles: layer.tiles.map(placedTileToJson),
  };
}

function fluidsGridHasContent(fluids: number[][]): boolean {
  return fluids.some((row) => row.some((cell) => cell !== 0));
}

export function serializeLevel(level: LevelData): string {
  const normalized = ensureTileLayers(ensureLevelGrids(level));
  const json: Record<string, unknown> = {
    format: LEVEL_FORMAT,
    version: LEVEL_VERSION,
    name: normalized.name,
    grid_size: normalized.grid_size,
    width: normalized.width,
    height: normalized.height,
    tile_layers: normalized.tile_layers.map(tileLayerToJson),
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
