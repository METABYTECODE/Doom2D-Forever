import { useEffect, useRef, useState } from "react";
import { animatedTileFrame } from "../lib/tile-animation";
import { constrainAxisLineEnd, cellSpanFootprint, hitObject, previewCellsForFill, previewCellsForLine, snapWorldPoint } from "../lib/geometry";
import { tileIndexToAtlasCell } from "../lib/tile-math";
import { pixelAxisLinePositions, pixelFillPositions } from "../lib/tile-pixel-tools";
import { tileDestRect } from "../lib/tile-render";
import { subGridDimensions, worldToSubCell } from "../lib/sub-grid";
import { cellKey, gridCellHasContent, parseCellKey } from "../lib/grid-tool-cells";
import {
  layerPickAtWorld,
  layerPickHasContent,
  selectDragStartAtPointer,
  type LayerPick,
} from "../lib/layer-rect-tools";
import {
  FLUID_ACID,
  FLUID_LAVA,
  FLUID_WATER,
  type EditorMode,
  type GridTool,
  type LevelData,
  type ObjectTool,
} from "../types/level";
import type { TilesetDef } from "../types/tileset";
import type { ModelData } from "../types/model";
import {
  colliderAabbFromPivot,
  modelColliderForObject,
  objectColliderAabb,
  playerPlacementPivot,
} from "../lib/object-collider";
import { blankModelPlacement } from "../lib/model-space";

interface Props {
  level: LevelData;
  mode: EditorMode;
  gridTool: GridTool;
  objectTool: ObjectTool;
  snapGrid: boolean;
  activeTilesetId: string;
  tilesets: Map<string, TilesetDef>;
  tilesetImages: Map<string, HTMLImageElement>;
  backgroundImage: HTMLImageElement | null;
  modelCatalog: Map<string, ModelData>;
  selectedObject: number;
  selectedPlacement: number;
  selectedPlacements: ReadonlySet<number>;
  selectedGridCells: ReadonlySet<string>;
  mapRevision: number;
  onPointer: (
    worldX: number,
    worldY: number,
    button: number,
    strokeFrom?: { x: number; y: number },
  ) => void;
  onStrokeBegin?: () => void;
  onStrokeEnd?: () => void;
  onObjectDrag: (index: number, worldX: number, worldY: number) => void;
  onPlacementsDrag: (indices: number[], anchorIndex: number, worldX: number, worldY: number) => void;
  onPlacementsDragEnd?: () => void;
  onGridCellsDrag: (cellKeys: string[], deltaCellX: number, deltaCellY: number) => void;
  onGridCellsDragEnd?: () => void;
  onClearLayerSelection?: () => void;
  onLayerPick: (pick: LayerPick | null, additive: boolean) => void;
  onRectSelect: (x0: number, y0: number, x1: number, y1: number, additive: boolean) => void;
  onRectErase: (x0: number, y0: number, x1: number, y1: number) => void;
  onLineTool: (x0: number, y0: number, x1: number, y1: number) => void;
  onFillTool: (x0: number, y0: number, x1: number, y1: number) => void;
}

const OBJECT_COLORS: Record<string, string> = {
  player: "#dc5a46",
  block: "#78aa5a",
};

const FLUID_FILL: Record<number, string> = {
  [FLUID_WATER]: "rgba(60, 140, 255, 0.48)",
  [FLUID_ACID]: "rgba(80, 220, 90, 0.45)",
  [FLUID_LAVA]: "rgba(255, 100, 40, 0.5)",
};

const FLUID_STROKE: Record<number, string> = {
  [FLUID_WATER]: "rgba(140, 200, 255, 0.9)",
  [FLUID_ACID]: "rgba(160, 255, 160, 0.9)",
  [FLUID_LAVA]: "rgba(255, 180, 100, 0.95)",
};

type HoverRect = { x: number; y: number; w: number; h: number };
type MarqueeRect = { x0: number; y0: number; x1: number; y1: number };
type GridRectDrag = MarqueeRect & { sx: number; sy: number; tool: "select" | "erase" };

type DrawState = {
  camera: { x: number; y: number; zoom: number };
  hover: HoverRect | null;
  marquee: MarqueeRect | null;
  level: LevelData;
  mode: EditorMode;
  gridTool: GridTool;
  objectTool: ObjectTool;
  snapGrid: boolean;
  activeTilesetId: string;
  tilesets: Map<string, TilesetDef>;
  tilesetImages: Map<string, HTMLImageElement>;
  backgroundImage: HTMLImageElement | null;
  modelCatalog: Map<string, ModelData>;
  selectedObject: number;
  selectedPlacement: number;
  selectedPlacements: ReadonlySet<number>;
  selectedGridCells: ReadonlySet<string>;
};

function hoverFootprint(
  state: DrawState,
  worldX: number,
  worldY: number,
  gridSize: number,
): HoverRect {
  if (
    (state.mode === "collision" || state.mode === "fluids") &&
    state.gridTool === "paint"
  ) {
    const cell = worldToSubCell(worldX, worldY, gridSize);
    return {
      x: cell.x * gridSize,
      y: cell.y * gridSize,
      w: gridSize,
      h: gridSize,
    };
  }

  const anchor = snapWorldPoint(worldX, worldY, gridSize, state.snapGrid);

  if (
    state.mode === "tiles" &&
    state.activeTilesetId &&
    state.gridTool !== "line" &&
    state.gridTool !== "fill"
  ) {
    const tileset = state.tilesets.get(state.activeTilesetId);
    if (tileset) {
      const dest = tileDestRect(anchor.x, anchor.y, tileset);
      return { x: dest.x, y: dest.y, w: dest.w, h: dest.h };
    }
  }

  if (state.mode === "objects" && state.objectTool === "place-player") {
    const playerModel = state.modelCatalog.get("player");
    const pivot = playerPlacementPivot(anchor.x, anchor.y, gridSize);
    if (playerModel) {
      const box = colliderAabbFromPivot(pivot.x, pivot.y, playerModel);
      return { x: box.x, y: box.y, w: box.width, h: box.height };
    }
    const blank = blankModelPlacement();
    const box = colliderAabbFromPivot(pivot.x, pivot.y, {
      format: "rivet-model",
      version: 3,
      id: "player",
      name: "player",
      resource_pack: "dev",
      pivot: blank.pivot,
      collider: blank.collider,
      animations: {},
    });
    return { x: box.x, y: box.y, w: box.width, h: box.height };
  }
  if (state.mode === "objects" && state.objectTool === "place-block") {
    return { x: anchor.x, y: anchor.y, w: 64, h: 64 };
  }
  return { x: anchor.x, y: anchor.y, w: gridSize, h: gridSize };
}

function hoverStrokeColor(mode: EditorMode): string {
  if (mode === "collision") return "rgba(255,200,100,1)";
  if (mode === "fluids") return "rgba(120,190,255,1)";
  return "rgba(110,168,255,0.9)";
}

function hoverFillColor(mode: EditorMode): string {
  if (mode === "collision") return "rgba(255,90,60,0.28)";
  if (mode === "fluids") return "rgba(80,160,255,0.28)";
  return "rgba(110,168,255,0.16)";
}

function drawFluidOverlay(
  ctx: CanvasRenderingContext2D,
  lvl: LevelData,
  mode: EditorMode,
  gridSize: number,
  cols: number,
  rows: number,
  gridScreen: number,
  toScreen: (wx: number, wy: number) => { x: number; y: number },
) {
  const fluidsActive = mode === "fluids";
  for (let gy = 0; gy < rows; gy++) {
    for (let gx = 0; gx < cols; gx++) {
      const fluid = lvl.fluids[gy]?.[gx] ?? 0;
      if (fluid === 0) continue;
      const p = toScreen(gx * gridSize, gy * gridSize);
      ctx.fillStyle = FLUID_FILL[fluid] ?? "rgba(120,120,255,0.4)";
      ctx.fillRect(p.x, p.y, gridScreen, gridScreen);
      ctx.strokeStyle = fluidsActive
        ? (FLUID_STROKE[fluid] ?? "rgba(180,180,255,0.8)")
        : (FLUID_STROKE[fluid] ?? "rgba(140,140,200,0.45)");
      ctx.lineWidth = fluidsActive ? 2 : 1;
      ctx.strokeRect(p.x + 0.5, p.y + 0.5, gridScreen - 1, gridScreen - 1);
    }
  }
}

function drawCollisionOverlay(
  ctx: CanvasRenderingContext2D,
  lvl: LevelData,
  mode: EditorMode,
  gridSize: number,
  cols: number,
  rows: number,
  gridScreen: number,
  toScreen: (wx: number, wy: number) => { x: number; y: number },
) {
  const collisionFill =
    mode === "collision" ? "rgba(255, 50, 50, 0.62)" : "rgba(255, 70, 70, 0.38)";
  const collisionStroke =
    mode === "collision" ? "rgba(255, 220, 120, 0.95)" : "rgba(255, 180, 100, 0.6)";

  for (let gy = 0; gy < rows; gy++) {
    for (let gx = 0; gx < cols; gx++) {
      if ((lvl.collision[gy]?.[gx] ?? 0) === 0) continue;
      const p = toScreen(gx * gridSize, gy * gridSize);
      ctx.fillStyle = collisionFill;
      ctx.fillRect(p.x, p.y, gridScreen, gridScreen);
      ctx.strokeStyle = collisionStroke;
      ctx.lineWidth = mode === "collision" ? 2 : 1;
      ctx.strokeRect(p.x + 0.5, p.y + 0.5, gridScreen - 1, gridScreen - 1);
    }
  }
}

function drawPreviewFootprints(
  ctx: CanvasRenderingContext2D,
  footprints: HoverRect[],
  cam: { zoom: number },
  toScreen: (wx: number, wy: number) => { x: number; y: number },
  fillStyle: string,
  strokeStyle: string,
) {
  for (const fp of footprints) {
    const p = toScreen(fp.x, fp.y);
    const w = fp.w * cam.zoom;
    const h = fp.h * cam.zoom;
    ctx.fillStyle = fillStyle;
    ctx.fillRect(p.x, p.y, w, h);
    ctx.strokeStyle = strokeStyle;
    ctx.lineWidth = 2;
    ctx.strokeRect(p.x, p.y, w, h);
  }
}

function computeLinePreview(
  mode: EditorMode,
  state: DrawState,
  lvl: LevelData,
  locked: MarqueeRect,
  snap: boolean,
): { cells: Array<{ x: number; y: number }> | null; footprints: HoverRect[] | null } {
  const { gridSize } = subGridDimensions(lvl);
  if (mode === "tiles" && state.activeTilesetId) {
    const ts = state.tilesets.get(state.activeTilesetId);
    const tw = ts?.tile_width ?? gridSize;
    const positions = pixelAxisLinePositions(locked.x0, locked.y0, locked.x1, locked.y1, tw);
    return {
      cells: null,
      footprints: positions.map((p) =>
        ts
          ? tileDestRect(p.x, p.y, ts)
          : { x: p.x, y: p.y, w: gridSize, h: gridSize },
      ),
    };
  }
  return {
    cells: previewCellsForLine(locked.x0, locked.y0, locked.x1, locked.y1, gridSize, snap),
    footprints: null,
  };
}

function computeFillPreview(
  mode: EditorMode,
  state: DrawState,
  lvl: LevelData,
  x0: number,
  y0: number,
  x1: number,
  y1: number,
): { cells: Array<{ x: number; y: number }> | null; footprints: HoverRect[] | null } {
  const { gridSize, cols, rows } = subGridDimensions(lvl);
  if (mode === "tiles" && state.activeTilesetId) {
    const ts = state.tilesets.get(state.activeTilesetId);
    const tw = ts?.tile_width ?? gridSize;
    const th = ts?.tile_height ?? gridSize;
    const positions = pixelFillPositions(x0, y0, x1, y1, tw, th);
    return {
      cells: null,
      footprints: positions.map((p) =>
        ts
          ? tileDestRect(p.x, p.y, ts)
          : { x: p.x, y: p.y, w: gridSize, h: gridSize },
      ),
    };
  }
  return {
    cells: previewCellsForFill(x0, y0, x1, y1, gridSize, cols, rows),
    footprints: null,
  };
}

function drawPreviewCells(
  ctx: CanvasRenderingContext2D,
  cells: Array<{ x: number; y: number }>,
  gridSize: number,
  gridScreen: number,
  toScreen: (wx: number, wy: number) => { x: number; y: number },
  fillStyle: string,
  strokeStyle: string,
) {
  for (const cell of cells) {
    const p = toScreen(cell.x * gridSize, cell.y * gridSize);
    ctx.fillStyle = fillStyle;
    ctx.fillRect(p.x, p.y, gridScreen, gridScreen);
    ctx.strokeStyle = strokeStyle;
    ctx.lineWidth = 2;
    ctx.strokeRect(p.x + 0.5, p.y + 0.5, gridScreen - 1, gridScreen - 1);
  }
}

function drawLayerSelection(
  ctx: CanvasRenderingContext2D,
  state: DrawState,
  lvl: LevelData,
  gridSize: number,
  cam: { zoom: number },
  elapsedMs: number,
  toScreen: (wx: number, wy: number) => { x: number; y: number },
  gridScreen: number,
) {
  if (state.mode === "tiles") {
    drawSelectedPlacements(ctx, state, lvl, gridSize, cam, elapsedMs, toScreen);
    return;
  }
  if (state.mode === "collision" || state.mode === "fluids") {
    drawSelectedGridCells(
      ctx,
      state.selectedGridCells,
      state.mode,
      lvl,
      gridSize,
      gridScreen,
      toScreen,
    );
  }
}

function drawSelectedGridCells(
  ctx: CanvasRenderingContext2D,
  selected: ReadonlySet<string>,
  editorMode: EditorMode,
  lvl: LevelData,
  gridSize: number,
  gridScreen: number,
  toScreen: (wx: number, wy: number) => { x: number; y: number },
) {
  for (const key of selected) {
    const cell = parseCellKey(key);
    if (!cell) continue;
    if (!gridCellHasContent(lvl, editorMode, cell.x, cell.y)) continue;
    const p = toScreen(cell.x * gridSize, cell.y * gridSize);
    ctx.fillStyle = "rgba(255, 246, 160, 0.38)";
    ctx.fillRect(p.x, p.y, gridScreen, gridScreen);
    ctx.strokeStyle = "#fff6a0";
    ctx.lineWidth = 2;
    ctx.strokeRect(p.x + 0.5, p.y + 0.5, gridScreen - 1, gridScreen - 1);
  }
}

function drawSelectedPlacements(
  ctx: CanvasRenderingContext2D,
  state: DrawState,
  lvl: LevelData,
  gridSize: number,
  cam: { zoom: number },
  elapsedMs: number,
  toScreen: (wx: number, wy: number) => { x: number; y: number },
) {
  const sortedLayers = [...lvl.tile_layers].sort((a, b) => a.z - b.z);
  let globalIndex = 0;
  for (const layer of sortedLayers) {
    for (const placement of layer.tiles) {
      const index = globalIndex;
      globalIndex += 1;
      const selected =
        state.selectedPlacements.has(index) || index === state.selectedPlacement;
      if (!selected) continue;

      const frame = animatedTileFrame(placement, elapsedMs);
      const footprintTs = state.tilesets.get(placement.tileset);
      const dest = footprintTs
        ? tileDestRect(placement.x, placement.y, footprintTs)
        : { x: placement.x, y: placement.y, w: gridSize, h: gridSize };
      const p = toScreen(dest.x, dest.y);
      const pw = dest.w * cam.zoom;
      const ph = dest.h * cam.zoom;

      ctx.strokeStyle = "#fff6a0";
      ctx.lineWidth = 2;
      ctx.strokeRect(p.x, p.y, pw, ph);
      const ap = toScreen(placement.x, placement.y);
      ctx.fillStyle = "#fff6a0";
      ctx.fillRect(ap.x - 2, ap.y - 2, 4, 4);
    }
  }
}

export function LevelCanvas({
  level,
  mode,
  gridTool,
  objectTool,
  snapGrid,
  activeTilesetId,
  tilesets,
  tilesetImages,
  backgroundImage,
  modelCatalog,
  selectedObject,
  selectedPlacement,
  selectedPlacements,
  selectedGridCells,
  mapRevision,
  onPointer,
  onStrokeBegin,
  onStrokeEnd,
  onObjectDrag,
  onPlacementsDrag,
  onPlacementsDragEnd,
  onGridCellsDrag,
  onGridCellsDragEnd,
  onClearLayerSelection,
  onLayerPick,
  onRectSelect,
  onRectErase,
  onLineTool,
  onFillTool,
}: Props) {
  const containerRef = useRef<HTMLDivElement>(null);
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const [camera, setCamera] = useState({ x: 320, y: 240, zoom: 2 });

  const paintingRef = useRef(false);
  const strokeButtonRef = useRef(0);
  const lastStrokeRef = useRef<{ x: number; y: number } | null>(null);
  const spaceRef = useRef(false);
  const panRef = useRef({ active: false, sx: 0, sy: 0, cx: 0, cy: 0 });
  const dragRef = useRef<
    | { kind: "object"; index: number; offsetX: number; offsetY: number }
    | {
        kind: "placements";
        indices: number[];
        anchorIndex: number;
        grabSnappedX: number;
        grabSnappedY: number;
        anchorStartX: number;
        anchorStartY: number;
      }
    | {
        kind: "gridCells";
        grabCellX: number;
        grabCellY: number;
        lastDeltaX: number;
        lastDeltaY: number;
        cellKeys: string[];
      }
    | null
  >(null);
  const marqueeRef = useRef<MarqueeRect | null>(null);
  const lineRef = useRef<MarqueeRect | null>(null);
  const fillRef = useRef<MarqueeRect | null>(null);
  const gridRectRef = useRef<GridRectDrag | null>(null);
  const previewCellsRef = useRef<Array<{ x: number; y: number }> | null>(null);
  const previewFootprintsRef = useRef<HoverRect[] | null>(null);
  const hoverRef = useRef<HoverRect | null>(null);
  const animationStartRef = useRef(performance.now());
  const cameraRef = useRef(camera);
  const levelRef = useRef(level);
  const pendingPlacementDragRef = useRef<{
    indices: number[];
    anchorIndex: number;
    worldX: number;
    worldY: number;
  } | null>(null);
  const placementDragRafRef = useRef(0);
  const drawStateRef = useRef<DrawState>({
    camera,
    hover: null,
    marquee: null,
    level,
    mode,
    gridTool,
    objectTool,
    snapGrid,
    activeTilesetId,
    tilesets,
    tilesetImages,
    backgroundImage,
    modelCatalog,
    selectedObject,
    selectedPlacement,
    selectedPlacements,
    selectedGridCells,
  });

  const applyLinePreview = (locked: MarqueeRect) => {
    const preview = computeLinePreview(mode, drawStateRef.current, levelRef.current, locked, snapGrid);
    previewCellsRef.current = preview.cells;
    previewFootprintsRef.current = preview.footprints;
  };

  const applyFillPreview = (x0: number, y0: number, x1: number, y1: number) => {
    const preview = computeFillPreview(mode, drawStateRef.current, levelRef.current, x0, y0, x1, y1);
    previewCellsRef.current = preview.cells;
    previewFootprintsRef.current = preview.footprints;
  };

  cameraRef.current = camera;
  levelRef.current = level;
  Object.assign(drawStateRef.current, {
    camera,
    hover: hoverRef.current,
    marquee: marqueeRef.current ?? lineRef.current ?? fillRef.current,
    level,
    mode,
    gridTool,
    objectTool,
    snapGrid,
    activeTilesetId,
    tilesets,
    tilesetImages,
    backgroundImage,
    modelCatalog,
    selectedObject,
    selectedPlacement,
    selectedPlacements,
    selectedGridCells,
  });

  useEffect(() => {
    setCamera({
      x: level.width / 2,
      y: level.height / 2,
      zoom: 2,
    });
  }, [mapRevision]);

  useEffect(() => {
    const onKeyDown = (e: KeyboardEvent) => {
      if (e.code === "Space") spaceRef.current = true;
    };
    const onKeyUp = (e: KeyboardEvent) => {
      if (e.code === "Space") spaceRef.current = false;
    };
    window.addEventListener("keydown", onKeyDown);
    window.addEventListener("keyup", onKeyUp);
    return () => {
      window.removeEventListener("keydown", onKeyDown);
      window.removeEventListener("keyup", onKeyUp);
    };
  }, []);

  useEffect(() => {
    const container = containerRef.current;
    if (!container) return;

    const onWheel = (e: WheelEvent) => {
      e.preventDefault();
      e.stopPropagation();
      const factor = e.deltaY < 0 ? 1.1 : 0.9;
      setCamera((c) => ({
        ...c,
        zoom: Math.min(8, Math.max(0.25, c.zoom * factor)),
      }));
    };

    container.addEventListener("wheel", onWheel, { passive: false });
    return () => container.removeEventListener("wheel", onWheel);
  }, []);

  useEffect(() => {
    const canvas = canvasRef.current;
    const container = containerRef.current;
    if (!canvas || !container) return;

    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    const resize = () => {
      const rect = container.getBoundingClientRect();
      const dpr = window.devicePixelRatio || 1;
      canvas.width = Math.floor(rect.width * dpr);
      canvas.height = Math.floor(rect.height * dpr);
      canvas.style.width = `${rect.width}px`;
      canvas.style.height = `${rect.height}px`;
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    };

    resize();
    const observer = new ResizeObserver(resize);
    observer.observe(container);

    let frame = 0;
    const draw = () => {
      const state = drawStateRef.current;
      const cam = cameraRef.current;
      const lvl = levelRef.current;
      const hover = hoverRef.current;
      const marquee = marqueeRef.current ?? lineRef.current ?? fillRef.current;
      const previewCells = previewCellsRef.current;
      const previewFootprints = previewFootprintsRef.current;
      const { gridSize, cols, rows } = subGridDimensions(lvl);
      const rect = container.getBoundingClientRect();
      const w = rect.width;
      const h = rect.height;

      ctx.fillStyle = "#0c0e14";
      ctx.fillRect(0, 0, w, h);

      const toScreen = (wx: number, wy: number) => ({
        x: w * 0.5 + (wx - cam.x) * cam.zoom,
        y: h * 0.5 + (wy - cam.y) * cam.zoom,
      });

      const gridScreen = gridSize * cam.zoom;
      const elapsedMs = performance.now() - animationStartRef.current;
      const worldW = lvl.width;
      const worldH = lvl.height;

      if (state.backgroundImage && lvl.background) {
        const topLeft = toScreen(0, 0);
        ctx.drawImage(
          state.backgroundImage,
          topLeft.x,
          topLeft.y,
          worldW * cam.zoom,
          worldH * cam.zoom,
        );
      }

      for (let gy = 0; gy < rows; gy++) {
        for (let gx = 0; gx < cols; gx++) {
          const p = toScreen(gx * gridSize, gy * gridSize);
          if (!state.backgroundImage || !lvl.background) {
            ctx.fillStyle = (gx + gy) % 2 === 0 ? "#151820" : "#12151c";
            ctx.fillRect(p.x, p.y, gridScreen, gridScreen);
          }
          ctx.strokeStyle =
            state.mode === "collision" || state.mode === "fluids"
              ? "rgba(120, 140, 180, 0.38)"
              : "rgba(80, 90, 110, 0.18)";
          ctx.lineWidth = 1;
          ctx.strokeRect(p.x + 0.5, p.y + 0.5, gridScreen - 1, gridScreen - 1);
        }
      }

      ctx.strokeStyle = "rgba(120, 140, 180, 0.55)";
      ctx.lineWidth = 2;
      const bounds = toScreen(0, 0);
      ctx.strokeRect(bounds.x, bounds.y, worldW * cam.zoom, worldH * cam.zoom);

      const sortedLayers = [...lvl.tile_layers].sort((a, b) => a.z - b.z);
      for (const layer of sortedLayers) {
        layer.tiles.forEach((placement) => {
        const frame = animatedTileFrame(placement, elapsedMs);
        const ts = state.tilesets.get(frame.tileset);
        const image = state.tilesetImages.get(frame.tileset);
        const footprintTs = state.tilesets.get(placement.tileset);
        const dest = footprintTs
          ? tileDestRect(placement.x, placement.y, footprintTs)
          : { x: placement.x, y: placement.y, w: gridSize, h: gridSize };
        const p = toScreen(dest.x, dest.y);
        const pw = dest.w * cam.zoom;
        const ph = dest.h * cam.zoom;

        if (ts && image) {
          const { col, row } = tileIndexToAtlasCell(frame.id, ts.columns);
          ctx.imageSmoothingEnabled = false;
          ctx.drawImage(
            image,
            col * ts.tile_width,
            row * ts.tile_height,
            ts.tile_width,
            ts.tile_height,
            p.x,
            p.y,
            pw,
            ph,
          );
        } else {
          ctx.fillStyle = "#555";
          ctx.fillRect(p.x, p.y, pw, ph);
        }
        });
      }

      lvl.objects.forEach((object, index) => {
        const box = objectColliderAabb(object, state.modelCatalog);
        const p = toScreen(box.x, box.y);
        const ow = box.width * cam.zoom;
        const oh = box.height * cam.zoom;
        ctx.fillStyle = OBJECT_COLORS[object.type] ?? "#c8c850";
        ctx.globalAlpha = index === state.selectedObject ? 1 : 0.85;
        ctx.fillRect(p.x, p.y, ow, oh);
        ctx.globalAlpha = 1;
        ctx.strokeStyle = index === state.selectedObject ? "#fff6a0" : "rgba(255,255,255,0.35)";
        ctx.lineWidth = index === state.selectedObject ? 2 : 1;
        ctx.strokeRect(p.x, p.y, ow, oh);
      });

      drawFluidOverlay(ctx, lvl, state.mode, gridSize, cols, rows, gridScreen, toScreen);
      drawCollisionOverlay(ctx, lvl, state.mode, gridSize, cols, rows, gridScreen, toScreen);
      if (state.gridTool === "select" && state.mode !== "objects") {
        drawLayerSelection(ctx, state, lvl, gridSize, cam, elapsedMs, toScreen, gridScreen);
      }

      if (previewFootprints && previewFootprints.length > 0) {
        drawPreviewFootprints(
          ctx,
          previewFootprints,
          cam,
          toScreen,
          "rgba(110, 168, 255, 0.22)",
          "rgba(110, 168, 255, 0.95)",
        );
      } else if (previewCells && previewCells.length > 0) {
        drawPreviewCells(
          ctx,
          previewCells,
          gridSize,
          gridScreen,
          toScreen,
          "rgba(110, 168, 255, 0.22)",
          "rgba(110, 168, 255, 0.95)",
        );
      } else if (marquee) {
        const fp = cellSpanFootprint(marquee.x0, marquee.y0, marquee.x1, marquee.y1, gridSize);
        const p = toScreen(fp.x, fp.y);
        const rw = fp.w * cam.zoom;
        const rh = fp.h * cam.zoom;
        ctx.fillStyle = "rgba(110, 168, 255, 0.12)";
        ctx.fillRect(p.x, p.y, rw, rh);
        ctx.strokeStyle = "rgba(110, 168, 255, 0.95)";
        ctx.lineWidth = 2;
        ctx.setLineDash([6, 4]);
        ctx.strokeRect(p.x, p.y, rw, rh);
        ctx.setLineDash([]);
      }

      if (hover) {
        const hp = toScreen(hover.x, hover.y);
        const hw = hover.w * cam.zoom;
        const hh = hover.h * cam.zoom;
        ctx.fillStyle = hoverFillColor(state.mode);
        ctx.fillRect(hp.x, hp.y, hw, hh);
        ctx.strokeStyle = hoverStrokeColor(state.mode);
        ctx.lineWidth = 2;
        ctx.strokeRect(hp.x, hp.y, hw, hh);
      }

      ctx.fillStyle = "rgba(200,210,230,0.7)";
      ctx.font = "11px JetBrains Mono, monospace";
      const hoverText = hover ? ` · ${Math.round(hover.x)},${Math.round(hover.y)}` : "";
      ctx.fillText(
        `${state.mode}/${state.gridTool} · ${Math.round(cam.zoom * 100)}%${hoverText} · MMB/Space pan`,
        12,
        h - 10,
      );

      frame = requestAnimationFrame(draw);
    };

    frame = requestAnimationFrame(draw);
    return () => {
      cancelAnimationFrame(frame);
      if (placementDragRafRef.current) {
        cancelAnimationFrame(placementDragRafRef.current);
        placementDragRafRef.current = 0;
      }
      observer.disconnect();
    };
  }, []);

  const screenToWorld = (clientX: number, clientY: number) => {
    const rect = containerRef.current!.getBoundingClientRect();
    const cam = cameraRef.current;
    const sx = clientX - rect.left;
    const sy = clientY - rect.top;
    const w = rect.width;
    const h = rect.height;
    return {
      x: cam.x + (sx - w * 0.5) / cam.zoom,
      y: cam.y + (sy - h * 0.5) / cam.zoom,
    };
  };

  const updateHover = (clientX: number, clientY: number) => {
    const world = screenToWorld(clientX, clientY);
    const { gridSize } = subGridDimensions(level);
    const next = hoverFootprint(drawStateRef.current, world.x, world.y, gridSize);
    const prev = hoverRef.current;
    if (!prev || prev.x !== next.x || prev.y !== next.y || prev.w !== next.w || prev.h !== next.h) {
      hoverRef.current = next;
      drawStateRef.current.hover = hoverRef.current;
    }
  };

  const strokeActive = gridTool === "paint" && mode !== "objects";

  const paintStroke = (clientX: number, clientY: number, button: number, isStrokeStart: boolean) => {
    const world = screenToWorld(clientX, clientY);
    const { level: lvl, snapGrid: snap } = drawStateRef.current;
    const { gridSize } = subGridDimensions(lvl);
    const pos = snapWorldPoint(world.x, world.y, gridSize, snap);

    if (isStrokeStart) {
      lastStrokeRef.current = pos;
      onPointer(pos.x, pos.y, button);
      return;
    }

    const last = lastStrokeRef.current;
    if (!last || last.x !== pos.x || last.y !== pos.y) {
      onPointer(pos.x, pos.y, button, last ?? undefined);
      lastStrokeRef.current = pos;
    }
  };

  const clearDragPreview = () => {
    marqueeRef.current = null;
    lineRef.current = null;
    fillRef.current = null;
    gridRectRef.current = null;
    previewCellsRef.current = null;
    previewFootprintsRef.current = null;
    drawStateRef.current.marquee = null;
  };

  return (
    <div
      ref={containerRef}
      className="canvas-viewport"
      onContextMenu={(e) => e.preventDefault()}
      onPointerDown={(e) => {
        e.preventDefault();
        (e.currentTarget as HTMLDivElement).setPointerCapture(e.pointerId);
        const world = screenToWorld(e.clientX, e.clientY);

        if (e.button === 1 || (e.button === 0 && spaceRef.current)) {
          panRef.current = {
            active: true,
            sx: e.clientX,
            sy: e.clientY,
            cx: cameraRef.current.x,
            cy: cameraRef.current.y,
          };
          return;
        }

        if (mode === "objects" && objectTool === "select" && e.button === 0) {
          const hit = hitObject(level.objects, world.x, world.y, modelCatalog);
          if (hit >= 0) {
            const object = level.objects[hit];
            dragRef.current = {
              kind: "object",
              index: hit,
              offsetX: world.x - object.x,
              offsetY: world.y - object.y,
            };
            onPointer(world.x, world.y, e.button);
            return;
          }
        }

        if (
          (gridTool === "select" || gridTool === "erase") &&
          e.button === 0 &&
          mode !== "objects"
        ) {
          if (gridTool === "select") {
            const pick = layerPickAtWorld(level, mode, tilesets, world.x, world.y);
            const dragStart = selectDragStartAtPointer(
              level,
              mode,
              tilesets,
              world.x,
              world.y,
              e.shiftKey,
              snapGrid,
              selectedPlacements,
              selectedGridCells,
            );
            const startingContentDrag = dragStart !== null && !e.shiftKey;

            if (e.shiftKey && layerPickHasContent(pick)) {
              onLayerPick(pick, true);
              if (pick.tileIndex !== null) {
                const next = new Set(drawStateRef.current.selectedPlacements);
                if (next.has(pick.tileIndex)) next.delete(pick.tileIndex);
                else next.add(pick.tileIndex);
                drawStateRef.current.selectedPlacements = next;
                drawStateRef.current.selectedGridCells = new Set();
              } else if (pick.cellX >= 0) {
                const key = cellKey(pick.cellX, pick.cellY);
                const next = new Set(drawStateRef.current.selectedGridCells);
                if (next.has(key)) next.delete(key);
                else next.add(key);
                drawStateRef.current.selectedGridCells = next;
                drawStateRef.current.selectedPlacements = new Set();
              }
            } else if (layerPickHasContent(pick)) {
              if (
                pick.tileIndex !== null &&
                !selectedPlacements.has(pick.tileIndex)
              ) {
                onLayerPick(pick, false);
                drawStateRef.current.selectedPlacements = new Set([pick.tileIndex]);
                drawStateRef.current.selectedGridCells = new Set();
              } else if (pick.cellX >= 0) {
                const key = cellKey(pick.cellX, pick.cellY);
                if (!selectedGridCells.has(key)) {
                  onLayerPick(pick, false);
                  drawStateRef.current.selectedGridCells = new Set([key]);
                  drawStateRef.current.selectedPlacements = new Set();
                }
              }
            } else if (!e.shiftKey) {
              onClearLayerSelection?.();
              drawStateRef.current.selectedPlacements = new Set();
              drawStateRef.current.selectedGridCells = new Set();
            }

            if (startingContentDrag) {
              if (dragStart!.kind === "tiles") {
                drawStateRef.current.selectedPlacements = new Set(dragStart!.indices);
                drawStateRef.current.selectedGridCells = new Set();
                dragRef.current = {
                  kind: "placements",
                  indices: dragStart!.indices,
                  anchorIndex: dragStart!.anchorIndex,
                  grabSnappedX: dragStart!.grabSnappedX,
                  grabSnappedY: dragStart!.grabSnappedY,
                  anchorStartX: dragStart!.anchorStartX,
                  anchorStartY: dragStart!.anchorStartY,
                };
              } else {
                drawStateRef.current.selectedGridCells = new Set(dragStart!.cellKeys);
                drawStateRef.current.selectedPlacements = new Set();
                dragRef.current = {
                  kind: "gridCells",
                  grabCellX: dragStart!.grabCellX,
                  grabCellY: dragStart!.grabCellY,
                  lastDeltaX: 0,
                  lastDeltaY: 0,
                  cellKeys: dragStart!.cellKeys,
                };
              }
              onStrokeBegin?.();
            }

            if (!startingContentDrag) {
              gridRectRef.current = {
                x0: world.x,
                y0: world.y,
                x1: world.x,
                y1: world.y,
                sx: e.clientX,
                sy: e.clientY,
                tool: gridTool,
              };
              marqueeRef.current = {
                x0: world.x,
                y0: world.y,
                x1: world.x,
                y1: world.y,
              };
              drawStateRef.current.marquee = marqueeRef.current;
            } else {
              gridRectRef.current = null;
              marqueeRef.current = null;
              drawStateRef.current.marquee = null;
            }
            return;
          }

          gridRectRef.current = {
            x0: world.x,
            y0: world.y,
            x1: world.x,
            y1: world.y,
            sx: e.clientX,
            sy: e.clientY,
            tool: gridTool,
          };
          marqueeRef.current = { x0: world.x, y0: world.y, x1: world.x, y1: world.y };
          drawStateRef.current.marquee = marqueeRef.current;
          return;
        }

        if (gridTool === "line" && e.button === 0 && mode !== "objects") {
          const { gridSize } = subGridDimensions(level);
          const s = snapWorldPoint(world.x, world.y, gridSize, snapGrid);
          lineRef.current = { x0: s.x, y0: s.y, x1: s.x, y1: s.y };
          applyLinePreview(lineRef.current);
          drawStateRef.current.marquee = lineRef.current;
          hoverRef.current = null;
          drawStateRef.current.hover = null;
          return;
        }

        if (gridTool === "fill" && e.button === 0 && mode !== "objects") {
          const { gridSize, cols, rows } = subGridDimensions(level);
          const s = snapWorldPoint(world.x, world.y, gridSize, snapGrid);
          fillRef.current = { x0: s.x, y0: s.y, x1: s.x, y1: s.y };
          applyFillPreview(s.x, s.y, s.x, s.y);
          drawStateRef.current.marquee = fillRef.current;
          hoverRef.current = null;
          drawStateRef.current.hover = null;
          return;
        }

        if (strokeActive && e.button === 0 && mode !== "objects") {
          paintingRef.current = true;
          strokeButtonRef.current = e.button;
          onStrokeBegin?.();
          paintStroke(e.clientX, e.clientY, e.button, true);
          return;
        }

        onPointer(world.x, world.y, e.button);
      }}
      onPointerMove={(e) => {
        const world = screenToWorld(e.clientX, e.clientY);
        const lvl = drawStateRef.current.level;
        const { gridSize } = subGridDimensions(lvl);
        const snap = drawStateRef.current.snapGrid;

        if (
          !lineRef.current &&
          !fillRef.current &&
          dragRef.current?.kind !== "placements" &&
          dragRef.current?.kind !== "gridCells"
        ) {
          updateHover(e.clientX, e.clientY);
        }

        if (panRef.current.active) {
          const cam = cameraRef.current;
          const dx = (e.clientX - panRef.current.sx) / cam.zoom;
          const dy = (e.clientY - panRef.current.sy) / cam.zoom;
          setCamera({ ...cam, x: panRef.current.cx - dx, y: panRef.current.cy - dy });
          return;
        }

        const contentDragging =
          dragRef.current?.kind === "placements" || dragRef.current?.kind === "gridCells";

        if (
          marqueeRef.current &&
          !lineRef.current &&
          !fillRef.current &&
          !contentDragging
        ) {
          marqueeRef.current = { ...marqueeRef.current, x1: world.x, y1: world.y };
          drawStateRef.current.marquee = marqueeRef.current;
        }
        if (
          gridRectRef.current &&
          !contentDragging
        ) {
          gridRectRef.current = { ...gridRectRef.current, x1: world.x, y1: world.y };
          marqueeRef.current = {
            x0: gridRectRef.current.x0,
            y0: gridRectRef.current.y0,
            x1: gridRectRef.current.x1,
            y1: gridRectRef.current.y1,
          };
          drawStateRef.current.marquee = marqueeRef.current;
        }
        if (lineRef.current) {
          const locked = constrainAxisLineEnd(
            lineRef.current.x0,
            lineRef.current.y0,
            world.x,
            world.y,
            gridSize,
            snap,
          );
          lineRef.current = locked;
          applyLinePreview(locked);
          drawStateRef.current.marquee = lineRef.current;
        }
        if (fillRef.current) {
          const snapped = snapWorldPoint(world.x, world.y, gridSize, snap);
          fillRef.current = { ...fillRef.current, x1: snapped.x, y1: snapped.y };
          applyFillPreview(
            fillRef.current.x0,
            fillRef.current.y0,
            fillRef.current.x1,
            fillRef.current.y1,
          );
          drawStateRef.current.marquee = fillRef.current;
        }

        if (dragRef.current?.kind === "object") {
          onObjectDrag(
            dragRef.current.index,
            world.x - dragRef.current.offsetX,
            world.y - dragRef.current.offsetY,
          );
          return;
        }

        if (dragRef.current?.kind === "placements") {
          const cursor = snapWorldPoint(world.x, world.y, gridSize, snap);
          const targetX =
            dragRef.current.anchorStartX + (cursor.x - dragRef.current.grabSnappedX);
          const targetY =
            dragRef.current.anchorStartY + (cursor.y - dragRef.current.grabSnappedY);
          pendingPlacementDragRef.current = {
            indices: dragRef.current.indices,
            anchorIndex: dragRef.current.anchorIndex,
            worldX: targetX,
            worldY: targetY,
          };
          const dragHover = hoverFootprint(
            drawStateRef.current,
            targetX,
            targetY,
            gridSize,
          );
          hoverRef.current = dragHover;
          drawStateRef.current.hover = dragHover;
          if (!placementDragRafRef.current) {
            placementDragRafRef.current = requestAnimationFrame(() => {
              placementDragRafRef.current = 0;
              const pending = pendingPlacementDragRef.current;
              if (!pending) return;
              onPlacementsDrag(
                pending.indices,
                pending.anchorIndex,
                pending.worldX,
                pending.worldY,
              );
            });
          }
          return;
        }

        if (dragRef.current?.kind === "gridCells") {
          const cursorCell = worldToSubCell(world.x, world.y, gridSize);
          const deltaX = cursorCell.x - dragRef.current.grabCellX;
          const deltaY = cursorCell.y - dragRef.current.grabCellY;
          if (
            deltaX === dragRef.current.lastDeltaX &&
            deltaY === dragRef.current.lastDeltaY
          ) {
            return;
          }
          dragRef.current = {
            ...dragRef.current,
            lastDeltaX: deltaX,
            lastDeltaY: deltaY,
          };
          onGridCellsDrag(dragRef.current.cellKeys, deltaX, deltaY);
          return;
        }

        if (!paintingRef.current) return;
        const button = e.buttons & 1 ? strokeButtonRef.current : e.button;
        paintStroke(e.clientX, e.clientY, button, false);
      }}
      onPointerUp={(e) => {
        const world = screenToWorld(e.clientX, e.clientY);
        const { gridSize } = subGridDimensions(level);

        if (gridRectRef.current) {
          const r = gridRectRef.current;
          const marqueeMoved = Math.hypot(e.clientX - r.sx, e.clientY - r.sy) > 2;
          const contentDragged =
            dragRef.current?.kind === "placements" ||
            (dragRef.current?.kind === "gridCells" &&
              (dragRef.current.lastDeltaX !== 0 || dragRef.current.lastDeltaY !== 0));

          if (r.tool === "select" && marqueeMoved && !contentDragged) {
            onRectSelect(r.x0, r.y0, r.x1, r.y1, e.shiftKey);
          } else if (r.tool === "erase") {
            onRectErase(r.x0, r.y0, r.x1, r.y1);
          }
        } else if (lineRef.current) {
          const { x0, y0 } = lineRef.current;
          const end = constrainAxisLineEnd(x0, y0, world.x, world.y, gridSize, snapGrid);
          try {
            onStrokeBegin?.();
            onLineTool(end.x0, end.y0, end.x1, end.y1);
          } finally {
            onStrokeEnd?.();
          }
        } else if (fillRef.current) {
          const { x0, y0 } = fillRef.current;
          const end = snapWorldPoint(world.x, world.y, gridSize, snapGrid);
          onStrokeBegin?.();
          onFillTool(x0, y0, end.x, end.y);
          onStrokeEnd?.();
        }

        if (dragRef.current?.kind === "placements" || dragRef.current?.kind === "gridCells") {
          onStrokeEnd?.();
        }

        if (paintingRef.current) {
          onStrokeEnd?.();
        }
        paintingRef.current = false;
        lastStrokeRef.current = null;
        panRef.current.active = false;
        dragRef.current = null;
        pendingPlacementDragRef.current = null;
        if (placementDragRafRef.current) {
          cancelAnimationFrame(placementDragRafRef.current);
          placementDragRafRef.current = 0;
        }
        onPlacementsDragEnd?.();
        onGridCellsDragEnd?.();
        clearDragPreview();
        try {
          (e.currentTarget as HTMLDivElement).releasePointerCapture(e.pointerId);
        } catch {
          /* ignore */
        }
      }}
      onPointerLeave={() => {
        if (paintingRef.current) {
          onStrokeEnd?.();
        }
        paintingRef.current = false;
        lastStrokeRef.current = null;
        panRef.current.active = false;
        dragRef.current = null;
        pendingPlacementDragRef.current = null;
        if (placementDragRafRef.current) {
          cancelAnimationFrame(placementDragRafRef.current);
          placementDragRafRef.current = 0;
        }
        onPlacementsDragEnd?.();
        onGridCellsDragEnd?.();
        clearDragPreview();
        hoverRef.current = null;
        drawStateRef.current.hover = null;
      }}
    >
      <canvas ref={canvasRef} />
    </div>
  );
}
