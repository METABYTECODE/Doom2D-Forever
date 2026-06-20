import type { EditorMode, LevelData } from "../types/level";
import type { TilesetDef } from "../types/tileset";
import { cellKey, cellsInWorldRect, gridCellHasContent } from "./grid-tool-cells";
import {
  findPlacementAtWorldInLevel,
  findPlacementRef,
  placementsInRectInLevel,
} from "./level-tile-layers";
import { subGridDimensions, worldToSubCell } from "./sub-grid";

/** Rect tool result — same shape for tiles, collision, and fluids. */
export type LayerRectContent = {
  tileIndices: number[];
  cellKeys: string[];
};

export type LayerPick = {
  tileIndex: number | null;
  cellX: number;
  cellY: number;
};

export type SelectDragStart =
  | {
      kind: "tiles";
      indices: number[];
      anchorIndex: number;
      grabSnappedX: number;
      grabSnappedY: number;
      anchorStartX: number;
      anchorStartY: number;
    }
  | { kind: "grid"; grabCellX: number; grabCellY: number; cellKeys: string[] };

export function layerContentCount(content: LayerRectContent): number {
  return content.tileIndices.length + content.cellKeys.length;
}

export function layerContentLabel(mode: EditorMode, count: number): string {
  const unit =
    mode === "tiles"
      ? count === 1
        ? "tile"
        : "tile(s)"
      : mode === "collision"
        ? count === 1
          ? "collision cell"
          : "collision cell(s)"
        : count === 1
          ? "fluid cell"
          : "fluid cell(s)";
  return `${count} ${unit}`;
}

function gridCellKeysInWorldRect(
  level: LevelData,
  mode: EditorMode,
  x0: number,
  y0: number,
  x1: number,
  y1: number,
  gridSize: number,
  cols: number,
  rows: number,
): string[] {
  return cellsInWorldRect(x0, y0, x1, y1, gridSize, cols, rows)
    .filter((cell) => gridCellHasContent(level, mode, cell.x, cell.y))
    .map((cell) => cellKey(cell.x, cell.y));
}

/**
 * Content inside a world rect for the active editor layer.
 * Tiles → placements overlapping rect. Collision/fluids → occupied cells only.
 */
export function layerContentInWorldRect(
  level: LevelData,
  mode: EditorMode,
  tilesets: Map<string, TilesetDef>,
  x0: number,
  y0: number,
  x1: number,
  y1: number,
): LayerRectContent {
  const empty: LayerRectContent = { tileIndices: [], cellKeys: [] };
  if (mode === "objects") return empty;

  const { gridSize, cols, rows } = subGridDimensions(level);

  if (mode === "tiles") {
    return {
      tileIndices: placementsInRectInLevel(level, tilesets, x0, y0, x1, y1, gridSize),
      cellKeys: [],
    };
  }

  return {
    tileIndices: [],
    cellKeys: gridCellKeysInWorldRect(level, mode, x0, y0, x1, y1, gridSize, cols, rows),
  };
}

/** Pick content under cursor on the active editor layer. */
export function layerPickAtWorld(
  level: LevelData,
  mode: EditorMode,
  tilesets: Map<string, TilesetDef>,
  worldX: number,
  worldY: number,
): LayerPick {
  const none: LayerPick = { tileIndex: null, cellX: -1, cellY: -1 };
  if (mode === "objects") return none;

  const { gridSize, cols, rows } = subGridDimensions(level);

  if (mode === "tiles") {
    const tileIndex = findPlacementAtWorldInLevel(level, tilesets, worldX, worldY);
    return { tileIndex: tileIndex >= 0 ? tileIndex : null, cellX: -1, cellY: -1 };
  }

  const cell = worldToSubCell(worldX, worldY, gridSize);
  if (cell.x < 0 || cell.y < 0 || cell.x >= cols || cell.y >= rows) return none;
  if (!gridCellHasContent(level, mode, cell.x, cell.y)) return none;
  return { tileIndex: null, cellX: cell.x, cellY: cell.y };
}

export function layerPickHasContent(pick: LayerPick): boolean {
  return pick.tileIndex !== null || pick.cellX >= 0;
}

/** Unified select-tool pointer down: pick + drag start, or null to begin marquee. */
export function selectDragStartAtPointer(
  level: LevelData,
  mode: EditorMode,
  tilesets: Map<string, TilesetDef>,
  worldX: number,
  worldY: number,
  shiftKey: boolean,
  snapGrid: boolean,
  selectedTileIndices: ReadonlySet<number>,
  selectedCellKeys: ReadonlySet<string>,
): SelectDragStart | null {
  if (mode === "objects") return null;

  const { gridSize, cols, rows } = subGridDimensions(level);
  const pick = layerPickAtWorld(level, mode, tilesets, worldX, worldY);
  const cell = worldToSubCell(worldX, worldY, gridSize);
  const inBounds = cell.x >= 0 && cell.y >= 0 && cell.x < cols && cell.y < rows;
  const cellKeyStr = inBounds ? cellKey(cell.x, cell.y) : "";
  const cellInSelection = inBounds && selectedCellKeys.has(cellKeyStr);

  if (pick.tileIndex !== null) {
    const placement = findPlacementRef(level, pick.tileIndex)?.tile;
    if (!placement) return null;

    const indices =
      selectedTileIndices.has(pick.tileIndex) && selectedTileIndices.size > 0
        ? [...selectedTileIndices]
        : shiftKey
          ? [...selectedTileIndices]
          : [pick.tileIndex];
    const dragIndices = indices.includes(pick.tileIndex)
      ? indices
      : [pick.tileIndex, ...indices];

    const grab = snapGrid
      ? {
          x: Math.floor(worldX / gridSize) * gridSize,
          y: Math.floor(worldY / gridSize) * gridSize,
        }
      : { x: worldX, y: worldY };

    return {
      kind: "tiles",
      indices: dragIndices,
      anchorIndex: pick.tileIndex,
      grabSnappedX: grab.x,
      grabSnappedY: grab.y,
      anchorStartX: placement.x,
      anchorStartY: placement.y,
    };
  }

  if (pick.cellX >= 0 || cellInSelection) {
    const pickKey = pick.cellX >= 0 ? cellKey(pick.cellX, pick.cellY) : "";
    let cellKeys: string[];
    if (pick.cellX >= 0) {
      if (selectedCellKeys.has(pickKey) && selectedCellKeys.size > 0) {
        cellKeys = [...selectedCellKeys];
      } else if (shiftKey) {
        cellKeys = selectedCellKeys.has(pickKey)
          ? [...selectedCellKeys]
          : [...selectedCellKeys, pickKey];
      } else {
        cellKeys = [pickKey];
      }
    } else {
      cellKeys = [...selectedCellKeys];
    }

    return {
      kind: "grid",
      grabCellX: pick.cellX >= 0 ? pick.cellX : cell.x,
      grabCellY: pick.cellY >= 0 ? pick.cellY : cell.y,
      cellKeys,
    };
  }

  return null;
}
