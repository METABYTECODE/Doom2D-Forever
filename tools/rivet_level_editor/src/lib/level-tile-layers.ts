import type { LevelData, PlacedTile, TileLayer } from "../types/level";
import type { TilesetDef } from "../types/tileset";
import { findPlacementAtWorld, placeTilesAtPositions } from "./tile-placements";
import { placementsInRect } from "./tile-selection";

export const DEFAULT_TILE_LAYER_ID = "main";
export const FOREGROUND_TILE_LAYER_ID = "foreground";

export const TILE_LAYER_PRESETS: Array<{ id: string; z: number; label: string }> = [
  { id: "background", z: -100, label: "Background" },
  { id: DEFAULT_TILE_LAYER_ID, z: 0, label: "Main" },
  { id: FOREGROUND_TILE_LAYER_ID, z: 100, label: "Foreground" },
];

export function defaultTileLayers(): TileLayer[] {
  return [
    { id: DEFAULT_TILE_LAYER_ID, z: 0, tiles: [] },
    { id: FOREGROUND_TILE_LAYER_ID, z: 100, tiles: [] },
  ];
}

export function ensureTileLayers(level: LevelData): LevelData {
  if (level.tile_layers.length === 0) {
    return { ...level, tile_layers: defaultTileLayers() };
  }
  return ensureStandardTileLayers(level);
}

/** Ensures main + foreground exist (v3 maps only had a flat tile list → main). */
export function ensureStandardTileLayers(level: LevelData): LevelData {
  const tile_layers = [...level.tile_layers];
  const hasId = (id: string) => tile_layers.some((layer) => layer.id === id);

  if (!hasId(DEFAULT_TILE_LAYER_ID)) {
    tile_layers.unshift({ id: DEFAULT_TILE_LAYER_ID, z: 0, tiles: [] });
  }
  if (!hasId(FOREGROUND_TILE_LAYER_ID)) {
    tile_layers.push({ id: FOREGROUND_TILE_LAYER_ID, z: 100, tiles: [] });
  }

  tile_layers.sort((a, b) => a.z - b.z);
  return { ...level, tile_layers };
}

export function tileLayerLabel(layer: TileLayer): string {
  const preset = TILE_LAYER_PRESETS.find((entry) => entry.id === layer.id);
  if (preset) {
    return `${preset.label} (z ${layer.z})`;
  }
  return `${layer.id} (z ${layer.z})`;
}

export function tileLayerHint(layerId: string): string {
  switch (layerId) {
    case "background":
      return "Behind everything on main (z −100)";
    case DEFAULT_TILE_LAYER_ID:
      return "Same depth as player — sorted by Y";
    case FOREGROUND_TILE_LAYER_ID:
      return "In front of player (z +100)";
    default:
      return "Lower z draws behind · higher z draws in front";
  }
}

export function flattenPlacements(level: LevelData): PlacedTile[] {
  return level.tile_layers.flatMap((layer) => layer.tiles);
}

export type PlacementRef = {
  globalIndex: number;
  layerIndex: number;
  tileIndex: number;
  layerId: string;
  z: number;
  tile: PlacedTile;
};

export function buildPlacementRefs(level: LevelData): PlacementRef[] {
  const refs: PlacementRef[] = [];
  let globalIndex = 0;
  for (let layerIndex = 0; layerIndex < level.tile_layers.length; layerIndex++) {
    const layer = level.tile_layers[layerIndex];
    for (let tileIndex = 0; tileIndex < layer.tiles.length; tileIndex++) {
      refs.push({
        globalIndex,
        layerIndex,
        tileIndex,
        layerId: layer.id,
        z: layer.z,
        tile: layer.tiles[tileIndex],
      });
      globalIndex += 1;
    }
  }
  return refs;
}

export function findPlacementRef(level: LevelData, globalIndex: number): PlacementRef | undefined {
  return buildPlacementRefs(level).find((ref) => ref.globalIndex === globalIndex);
}

export function findPlacementAtWorldInLevel(
  level: LevelData,
  tilesets: Map<string, TilesetDef>,
  worldX: number,
  worldY: number,
): number {
  const refs = buildPlacementRefs(level);
  const hits: PlacementRef[] = [];
  for (const ref of refs) {
    const hit = findPlacementAtWorld([ref.tile], tilesets, worldX, worldY);
    if (hit >= 0) {
      hits.push(ref);
    }
  }
  if (hits.length === 0) {
    return -1;
  }
  hits.sort((a, b) => b.z - a.z || b.tileIndex - a.tileIndex);
  return hits[0].globalIndex;
}

export function placementsInRectInLevel(
  level: LevelData,
  tilesets: Map<string, TilesetDef>,
  x0: number,
  y0: number,
  x1: number,
  y1: number,
  gridSize?: number,
): number[] {
  const refs = buildPlacementRefs(level);
  const hits: number[] = [];
  for (const ref of refs) {
    const localHits = placementsInRect([ref.tile], tilesets, x0, y0, x1, y1, gridSize);
    if (localHits.length > 0) {
      hits.push(ref.globalIndex);
    }
  }
  return hits;
}

export function removePlacementsByGlobalIndices(level: LevelData, indices: Set<number>): LevelData {
  if (indices.size === 0) {
    return level;
  }
  const refs = buildPlacementRefs(level);
  const removeByLayer = new Map<number, Set<number>>();
  for (const ref of refs) {
    if (!indices.has(ref.globalIndex)) {
      continue;
    }
    const bucket = removeByLayer.get(ref.layerIndex) ?? new Set<number>();
    bucket.add(ref.tileIndex);
    removeByLayer.set(ref.layerIndex, bucket);
  }

  const tile_layers = level.tile_layers.map((layer, layerIndex) => {
    const remove = removeByLayer.get(layerIndex);
    if (!remove) {
      return layer;
    }
    return {
      ...layer,
      tiles: layer.tiles.filter((_, tileIndex) => !remove.has(tileIndex)),
    };
  });
  return { ...level, tile_layers };
}

export function patchPlacementAtGlobal(
  level: LevelData,
  globalIndex: number,
  patch: Partial<PlacedTile>,
): LevelData {
  const ref = findPlacementRef(level, globalIndex);
  if (!ref) {
    return level;
  }
  const tile_layers = level.tile_layers.map((layer, layerIndex) => {
    if (layerIndex !== ref.layerIndex) {
      return layer;
    }
    return {
      ...layer,
      tiles: layer.tiles.map((tile, tileIndex) =>
        tileIndex === ref.tileIndex ? { ...tile, ...patch } : tile,
      ),
    };
  });
  return { ...level, tile_layers };
}

export function paintTilesOnLayer(
  level: LevelData,
  layerId: string,
  positions: Array<{ x: number; y: number }>,
  tilesetId: string,
  tileId: number,
): LevelData {
  const layerIndex = level.tile_layers.findIndex((layer) => layer.id === layerId);
  if (layerIndex < 0) {
    return level;
  }

  const layer = level.tile_layers[layerIndex];
  const nextTiles = placeTilesAtPositions(
    layer.tiles,
    positions,
    tilesetId,
    tileId,
    level.width,
    level.height,
  );
  if (nextTiles === layer.tiles) {
    return level;
  }

  const tile_layers = level.tile_layers.map((entry, index) =>
    index === layerIndex ? { ...entry, tiles: nextTiles } : entry,
  );
  return { ...level, tile_layers };
}

export function movePlacementsByGlobalIndices(
  level: LevelData,
  indices: number[],
  deltaX: number,
  deltaY: number,
): LevelData {
  if (indices.length === 0 || (deltaX === 0 && deltaY === 0)) {
    return level;
  }
  const move = new Set(indices);
  const tile_layers = level.tile_layers.map((layer, layerIndex) => {
    const refs = buildPlacementRefs({ ...level, tile_layers: level.tile_layers }).filter(
      (ref) => ref.layerIndex === layerIndex && move.has(ref.globalIndex),
    );
    if (refs.length === 0) {
      return layer;
    }
    const moved = new Set(refs.map((ref) => ref.tileIndex));
    return {
      ...layer,
      tiles: layer.tiles.map((tile, tileIndex) =>
        moved.has(tileIndex)
          ? { ...tile, x: tile.x + deltaX, y: tile.y + deltaY }
          : tile,
      ),
    };
  });
  return { ...level, tile_layers };
}

/** Drag preview: set positions from drag-start snapshot + total delta (not cumulative add). */
export function movePlacementsByGlobalIndicesFromSnapshot(
  level: LevelData,
  indices: number[],
  snapshot: ReadonlyMap<number, { x: number; y: number }>,
  deltaX: number,
  deltaY: number,
): LevelData {
  if (indices.length === 0 || (deltaX === 0 && deltaY === 0)) {
    return level;
  }

  let next = level;
  for (const index of indices) {
    const start = snapshot.get(index);
    if (!start) {
      continue;
    }
    next = patchPlacementAtGlobal(next, index, {
      x: start.x + deltaX,
      y: start.y + deltaY,
    });
  }
  return next;
}

export function findPlacementOnActiveLayerGlobal(
  level: LevelData,
  layerId: string,
  tilesets: Map<string, TilesetDef>,
  worldX: number,
  worldY: number,
): number {
  const refs = buildPlacementRefs(level).filter((ref) => ref.layerId === layerId);
  for (let index = refs.length - 1; index >= 0; index--) {
    const ref = refs[index];
    const hit = findPlacementAtWorld([ref.tile], tilesets, worldX, worldY);
    if (hit >= 0) {
      return ref.globalIndex;
    }
  }
  return -1;
}

export function eraseTilesOnLayer(
  level: LevelData,
  layerId: string,
  positions: Array<{ x: number; y: number }>,
): LevelData {
  if (positions.length === 0) {
    return level;
  }
  const layerIndex = level.tile_layers.findIndex((layer) => layer.id === layerId);
  if (layerIndex < 0) {
    return level;
  }
  const anchors = new Set(
    positions.map((position) => `${Math.round(position.x)}:${Math.round(position.y)}`),
  );
  const layer = level.tile_layers[layerIndex];
  const nextTiles = layer.tiles.filter(
    (tile) => !anchors.has(`${Math.round(tile.x)}:${Math.round(tile.y)}`),
  );
  if (nextTiles.length === layer.tiles.length) {
    return level;
  }
  const tile_layers = level.tile_layers.map((entry, index) =>
    index === layerIndex ? { ...entry, tiles: nextTiles } : entry,
  );
  return { ...level, tile_layers };
}

export function tileCount(level: LevelData): number {
  return flattenPlacements(level).length;
}
