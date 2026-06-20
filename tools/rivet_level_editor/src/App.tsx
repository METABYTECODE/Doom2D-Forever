import { useCallback, useEffect, useMemo, useRef, useState } from "react";

import { JsonPreview } from "./components/JsonPreview";

import {

  Inspector,

  MenuBar,

  OptionsBar,

  StatusBar,

  TilesetDock,

  ToolRail,

} from "./components/EditorLayout";

import { LevelCanvas } from "./components/LevelCanvas";

import { TilesetPickerModal } from "./components/TilesetPickerModal";

import { useLevelHistory } from "./hooks/useLevelHistory";

import { constrainAxisLineEnd, hitObject, snapPoint } from "./lib/geometry";

import { cellsInWorldRect, cellsOnWorldStroke, cellKey, gridCellHasContent, occupiedCellsInWorldRect, translateGridCellValues } from "./lib/grid-tool-cells";

import { gridLineCells, paintCollisionCells } from "./lib/level-collision";

import { rectSubCells } from "./lib/grid-rect";

import { FLUID_NONE } from "./types/level";

import { paintFluidCells } from "./lib/level-fluids";

import {

  createBlankLevel,

  downloadLevel,

  parseLevelJson,

  resizeLevel,
  setGridSize,
} from "./lib/level-io";

import { applyTilePaintLayers, paintFluidToId } from "./lib/tile-layers";

import { placementsInRect, canMovePlacements } from "./lib/tile-selection";
import {
  DEFAULT_TILE_LAYER_ID,
  findPlacementRef,
  flattenPlacements,
  paintTilesOnLayer,
  patchPlacementAtGlobal,
  placementsInRectInLevel,
  removePlacementsByGlobalIndices,
  movePlacementsByGlobalIndicesFromSnapshot,
  tileCount,
  buildPlacementRefs,
} from "./lib/level-tile-layers";

import { pixelAxisLinePositions, pixelFillPositions, pixelLinePositions } from "./lib/tile-pixel-tools";

import { subGridDimensions, worldToSubCell } from "./lib/sub-grid";

import { validateLevel } from "./lib/level-validation";

import { playerPlacementPivot } from "./lib/object-collider";

import {

  DEFAULT_PACK_ID,

  loadPackImage,

  loadResourcePack,
  loadModelCatalog,
  loadTilesetImage,

} from "./lib/resource-pack";

import type {

  EditorMode,

  FluidPaint,

  GridTool,

  LevelObject,

  ObjectTool,

  PaintFluidOption,

  PlacedTile,

} from "./types/level";

import { DEFAULT_FRAME_MS } from "./types/level";



export function App() {

  const { level, dirty, replace, update, beginStroke, endStroke, undo, redo, markSaved, canUndo, canRedo } =

    useLevelHistory(createBlankLevel());

  const [mode, setMode] = useState<EditorMode>("tiles");

  const [activeTileLayerId, setActiveTileLayerId] = useState(DEFAULT_TILE_LAYER_ID);

  const [gridTool, setGridToolState] = useState<GridTool>("select");

  const [fluidPaint, setFluidPaint] = useState<FluidPaint>("water");

  const [objectTool, setObjectTool] = useState<ObjectTool>("select");

  const [selectedGridCells, setSelectedGridCells] = useState<Set<string>>(new Set());

  const [paintCollision, setPaintCollision] = useState(false);

  const [paintFluid, setPaintFluid] = useState<PaintFluidOption>("none");

  const pack = useMemo(

    () => loadResourcePack(level.resource_pack || DEFAULT_PACK_ID),

    [level.resource_pack],

  );

  const modelCatalog = useMemo(

    () => loadModelCatalog(level.resource_pack || DEFAULT_PACK_ID),

    [level.resource_pack],

  );

  const [tilesets, setTilesets] = useState<Map<string, TilesetDef>>(() => pack.tilesets);

  const [tilesetImages, setTilesetImages] = useState<Map<string, HTMLImageElement>>(new Map());

  const [backgroundImage, setBackgroundImage] = useState<HTMLImageElement | null>(null);

  const [activeTilesetId, setActiveTilesetId] = useState("");

  const [selectedTileId, setSelectedTileId] = useState(0);

  const [selectedObject, setSelectedObject] = useState(-1);

  const [selectedPlacement, setSelectedPlacement] = useState(-1);

  const [selectedPlacements, setSelectedPlacements] = useState<Set<number>>(new Set());

  const clearTileSelection = useCallback(() => {
    setSelectedPlacements(new Set());
    setSelectedPlacement(-1);
  }, []);

  const clearGridSelection = useCallback(() => {
    setSelectedGridCells(new Set());
  }, []);

  const onGridToolChange = useCallback(
    (tool: GridTool) => {
      setGridToolState(tool);
      if (tool !== "select") {
        clearTileSelection();
        clearGridSelection();
      }
    },
    [clearGridSelection, clearTileSelection],
  );

  const onModeChange = useCallback(
    (nextMode: EditorMode) => {
      setMode(nextMode);
      clearTileSelection();
      clearGridSelection();
    },
    [clearGridSelection, clearTileSelection],
  );

  const [snapGrid, setSnapGrid] = useState(true);

  const [showJson, setShowJson] = useState(false);

  const [status, setStatus] = useState("New blank level");

  const [mapRevision, setMapRevision] = useState(0);

  const [tilePickerOpen, setTilePickerOpen] = useState(false);

  const [framePickerOpen, setFramePickerOpen] = useState(false);

  const [framePickerTilesetId, setFramePickerTilesetId] = useState("");

  const fileInputRef = useRef<HTMLInputElement>(null);

  const placementDragSnapshotRef = useRef<Map<number, { x: number; y: number }> | null>(null);

  const gridDragSnapshotRef = useRef<{
    keys: Set<string>;
    collision?: number[][];
    fluids?: number[][];
  } | null>(null);



  const activeTileset = tilesets.get(activeTilesetId) ?? null;



  useEffect(() => {

    setTilesets(pack.tilesets);

    const first = pack.tilesets.keys().next().value as string | undefined;

    if (first) setActiveTilesetId(first);



    void (async () => {

      const images = new Map<string, HTMLImageElement>();

      await Promise.all(

        [...pack.tilesets.values()].map(async (tileset) => {

          try {

            images.set(tileset.id, await loadTilesetImage(tileset));

          } catch {

            /* missing texture */

          }

        }),

      );

      setTilesetImages(images);

      const loaded = images.size;

      const total = pack.tilesets.size;

      const bg = pack.backgrounds.length;

      const music = pack.music.length;

      if (total === 0 && bg === 0 && music === 0) {

        setStatus(`Resource pack "${pack.manifest.id}" is empty — copy assets into src/resourcepacks/`);

      } else if (loaded < total) {

        setStatus(

          `Pack ${pack.manifest.name}: ${loaded}/${total} tilesets, ${bg} backgrounds, ${music} tracks`,

        );

      } else {

        setStatus(

          `Pack ${pack.manifest.name}: ${total} tilesets, ${bg} backgrounds, ${music} tracks`,

        );

      }

    })();

  }, [pack]);



  useEffect(() => {

    const asset = pack.backgrounds.find((entry) => entry.id === level.background);

    void loadPackImage(asset).then(setBackgroundImage);

  }, [level.background, pack.backgrounds]);

  useEffect(() => {
    setSelectedGridCells(new Set());
  }, [mode]);



  const exportLevel = useCallback(() => {

    const errors = validateLevel(level, modelCatalog);

    if (errors.length > 0) {

      setStatus(`Export blocked: ${errors[0]}`);

      return;

    }

    const filename = `${level.name || "level"}.level.json`;

    downloadLevel(level, filename);

    markSaved();

    setStatus(`Exported ${filename}`);

  }, [level, markSaved]);



  const deleteSelectedObject = useCallback(() => {

    if (selectedObject < 0) return;

    update((prev) => ({

      ...prev,

      objects: prev.objects.filter((_, i) => i !== selectedObject),

    }));

    setSelectedObject(-1);

    setStatus("Object deleted");

  }, [selectedObject, update]);



  const updatePlacement = useCallback(

    (index: number, patch: Partial<PlacedTile>) => {

      update((prev) => patchPlacementAtGlobal(prev, index, patch));

    },

    [update],

  );



  const openFramePicker = useCallback(() => {

    if (selectedPlacement < 0) return;

    const placement = findPlacementRef(level, selectedPlacement)?.tile;

    if (!placement) return;

    setFramePickerTilesetId(placement.tileset || activeTilesetId);

    setFramePickerOpen(true);

  }, [activeTilesetId, level, selectedPlacement]);



  const addFrameToPlacement = useCallback(

    (tilesetId: string, tileId: number) => {

      if (selectedPlacement < 0) return;

      update((prev) => {

        const current = findPlacementRef(prev, selectedPlacement)?.tile;

        if (!current) return prev;

        const frames = current.frames?.length

          ? [...current.frames]

          : [{ tileset: current.tileset, id: current.id }];

        frames.push({ tileset: tilesetId, id: tileId });

        return patchPlacementAtGlobal(prev, selectedPlacement, {

          frames,

          frame_ms: current.frame_ms ?? DEFAULT_FRAME_MS,

        });

      });

      setFramePickerOpen(false);

      setStatus(`Added frame ${tilesetId}:${tileId}`);

    },

    [selectedPlacement, update],

  );



  const deleteSelectedPlacement = useCallback(() => {

    const toRemove =

      selectedPlacements.size > 0

        ? selectedPlacements

        : selectedPlacement >= 0

          ? new Set([selectedPlacement])

          : new Set<number>();

    if (toRemove.size === 0) return;

    update((prev) => removePlacementsByGlobalIndices(prev, toRemove));

    setSelectedPlacement(-1);

    setSelectedPlacements(new Set());

    setStatus(`Deleted ${toRemove.size} tile(s)`);

  }, [selectedPlacement, selectedPlacements, update]);



  const selected = useMemo(

    () => (selectedObject >= 0 ? level.objects[selectedObject] : null),

    [level.objects, selectedObject],

  );



  const syncPrimarySelection = (next: Set<number>) => {

    setSelectedPlacements(next);

    const first = next.values().next().value as number | undefined;

    setSelectedPlacement(first ?? -1);

  };



  const onTilePick = (index: number, additive: boolean) => {

    if (index < 0) {

      if (!additive) syncPrimarySelection(new Set());

      return;

    }

    if (additive) {

      const next = new Set(selectedPlacements);

      if (next.has(index)) next.delete(index);

      else next.add(index);

      syncPrimarySelection(next);

      return;

    }

    syncPrimarySelection(new Set([index]));

  };



  const onMarqueeSelect = (x0: number, y0: number, x1: number, y1: number, additive: boolean) => {

    const { gridSize } = subGridDimensions(level);

    const hits = placementsInRectInLevel(level, tilesets, x0, y0, x1, y1, gridSize);

    if (additive) {

      const next = new Set(selectedPlacements);

      for (const index of hits) next.add(index);

      syncPrimarySelection(next);

    } else {

      syncPrimarySelection(new Set(hits));

    }

    setStatus(`Selected ${hits.length} tile(s)`);

  };



  const onRectSelect = (x0: number, y0: number, x1: number, y1: number, additive: boolean) => {

    if (mode === "tiles") {

      onMarqueeSelect(x0, y0, x1, y1, additive);

      return;

    }

    const { gridSize, cols, rows } = subGridDimensions(level);

    const cells = occupiedCellsInWorldRect(level, mode, x0, y0, x1, y1, gridSize, cols, rows);

    if (cells.length === 0) {

      if (!additive) setSelectedGridCells(new Set());

      setStatus(mode === "collision" ? "No collision in area" : "No fluid in area");

      return;

    }

    const next = additive ? new Set(selectedGridCells) : new Set<string>();

    for (const cell of cells) next.add(cellKey(cell.x, cell.y));

    setSelectedGridCells(next);

    setStatus(`Selected ${next.size} cell(s)`);

  };



  const onRectErase = (x0: number, y0: number, x1: number, y1: number) => {

    const { gridSize, cols, rows } = subGridDimensions(level);

    if (mode === "tiles") {

      const hits = placementsInRectInLevel(level, tilesets, x0, y0, x1, y1, gridSize);

      if (hits.length === 0) {

        setStatus("No tiles in area");

        return;

      }

      beginStroke();

      update((prev) => removePlacementsByGlobalIndices(prev, new Set(hits)));

      endStroke();

      clearTileSelection();

      setStatus(`Erased ${hits.length} tile(s)`);

      return;

    }

    const cells = cellsInWorldRect(x0, y0, x1, y1, gridSize, cols, rows);

    if (cells.length === 0) {

      setStatus("No cells in area");

      return;

    }

    beginStroke();

    if (mode === "collision") {

      applyCollisionCells(cells, 0);

      setStatus(`Erased ${cells.length} collision cell(s)`);

    } else if (mode === "fluids") {

      applyFluidCells(cells, FLUID_NONE);

      setStatus(`Erased ${cells.length} fluid cell(s)`);

    }

    endStroke();

    clearGridSelection();

  };



  const onGridCellPick = (cellX: number, cellY: number, additive: boolean) => {

    if (!gridCellHasContent(level, mode, cellX, cellY)) {

      if (!additive) setSelectedGridCells(new Set());

      return;

    }

    const key = cellKey(cellX, cellY);

    if (additive) {

      const next = new Set(selectedGridCells);

      if (next.has(key)) next.delete(key);

      else next.add(key);

      setSelectedGridCells(next);

      setStatus(`Grid selection: ${next.size} cell(s)`);

      return;

    }

    setSelectedGridCells(new Set([key]));

    setStatus("Selected 1 cell");

  };



  const placeAt = (worldX: number, worldY: number) => {

    const { gridSize } = subGridDimensions(level);

    return snapGrid ? snapPoint(worldX, worldY, gridSize) : { x: worldX, y: worldY };

  };



  const paintTilesAtPositions = (positions: Array<{ x: number; y: number }>) => {

    update((prev) => {

      const ts = tilesets.get(activeTilesetId);

      const painted = paintTilesOnLayer(

        prev,

        activeTileLayerId,

        positions,

        activeTilesetId,

        selectedTileId,

      );

      if (painted === prev) return prev;



      const collision = painted.collision.map((row) => [...row]);

      const fluids = painted.fluids.map((row) => [...row]);

      const { cols, rows, gridSize } = subGridDimensions(painted);

      let layersChanged = false;



      if (paintCollision || paintFluid !== "none") {

        const seen = new Set<string>();

        for (const { x, y } of positions) {

          const key = `${Math.round(x)}:${Math.round(y)}`;

          if (seen.has(key)) continue;

          seen.add(key);

          layersChanged =

            applyTilePaintLayers(

              collision,

              fluids,

              x,

              y,

              ts ?? null,

              paintCollision,

              paintFluid,

              cols,

              rows,

              gridSize,

            ) || layersChanged;

        }

      }



      if (!layersChanged && painted === prev) return prev;

      return { ...painted, collision, fluids };

    });

  };



  const applyCollisionCells = (

    cells: Array<{ x: number; y: number }>,

    value: number,

  ) => {

    update((prev) => {

      const { cols, rows } = subGridDimensions(prev);

      const collision = prev.collision.map((row) => [...row]);

      if (!paintCollisionCells(collision, cells, value, cols, rows)) return prev;

      return { ...prev, collision };

    });

  };



  const applyFluidCells = (cells: Array<{ x: number; y: number }>, value: number) => {

    update((prev) => {

      const { cols, rows } = subGridDimensions(prev);

      const fluids = prev.fluids.map((row) => [...row]);

      if (!paintFluidCells(fluids, cells, value, cols, rows)) return prev;

      return { ...prev, fluids };

    });

  };



  const deleteSelectedGridCells = useCallback(() => {

    if (selectedGridCells.size === 0) return;

    const cells = [...selectedGridCells]

      .map((key) => {

        const [xs, ys] = key.split(",");

        return { x: Number(xs), y: Number(ys) };

      })

      .filter((cell) => Number.isFinite(cell.x) && Number.isFinite(cell.y));

    if (cells.length === 0) return;

    beginStroke();

    if (mode === "collision") {

      applyCollisionCells(cells, 0);

      setStatus(`Cleared ${cells.length} collision cell(s)`);

    } else if (mode === "fluids") {

      applyFluidCells(cells, FLUID_NONE);

      setStatus(`Cleared ${cells.length} fluid cell(s)`);

    }

    endStroke();

    setSelectedGridCells(new Set());

  }, [mode, selectedGridCells, beginStroke, endStroke]);



  useEffect(() => {

    const onKeyDown = (e: KeyboardEvent) => {

      if (e.target instanceof HTMLInputElement || e.target instanceof HTMLTextAreaElement) return;



      if (e.ctrlKey && e.key.toLowerCase() === "z") {

        e.preventDefault();

        if (e.shiftKey) redo();

        else undo();

        setStatus(e.shiftKey ? "Redo" : "Undo");

        return;

      }

      if (e.ctrlKey && e.key.toLowerCase() === "s") {

        e.preventDefault();

        exportLevel();

        return;

      }

      if (e.key === "Delete") {

        if (mode === "tiles" && (selectedPlacements.size > 0 || selectedPlacement >= 0)) {

          deleteSelectedPlacement();

        } else if ((mode === "collision" || mode === "fluids") && selectedGridCells.size > 0) {

          deleteSelectedGridCells();

        } else if (selectedObject >= 0) deleteSelectedObject();

        return;

      }

      if (e.ctrlKey && e.key.toLowerCase() === "a" && mode === "tiles") {

        e.preventDefault();

        const all = new Set(buildPlacementRefs(level).map((ref) => ref.globalIndex));

        setSelectedPlacements(all);

        setSelectedPlacement(tileCount(level) > 0 ? 0 : -1);

        setStatus(`Selected ${all.size} tile(s)`);

        return;

      }

      if (e.key === "t") onModeChange("tiles");

      if (e.key === "c") onModeChange("collision");

      if (e.key === "f") onModeChange("fluids");

      if (e.key === "o") onModeChange("objects");

    };

    window.addEventListener("keydown", onKeyDown);

    return () => window.removeEventListener("keydown", onKeyDown);

  }, [

    deleteSelectedGridCells,

    deleteSelectedObject,

    deleteSelectedPlacement,

    exportLevel,

    level.tile_layers,

    mode,

    onModeChange,

    redo,

    selectedGridCells.size,

    selectedObject,

    selectedPlacement,

    selectedPlacements.size,

    undo,

  ]);



  const snapTileAnchors = (positions: Array<{ x: number; y: number }>) => {
    if (!snapGrid) return positions;
    return positions.map((p) => placeAt(p.x, p.y));
  };

  const onLineTool = (x0: number, y0: number, x1: number, y1: number) => {
    if (mode === "tiles") {
      const { gridSize } = subGridDimensions(level);
      const locked = constrainAxisLineEnd(x0, y0, x1, y1, gridSize, snapGrid);
      const tw = activeTileset?.tile_width ?? gridSize;
      const positions = snapTileAnchors(
        pixelAxisLinePositions(locked.x0, locked.y0, locked.x1, locked.y1, tw),
      );
      paintTilesAtPositions(positions);
      setStatus(`Line: ${positions.length} tile(s)`);
      return;
    }

    const { gridSize, cols, rows } = subGridDimensions(level);

    const c0 = worldToSubCell(x0, y0, gridSize);

    const c1 = worldToSubCell(x1, y1, gridSize);

    const cells = gridLineCells(c0.x, c0.y, c1.x, c1.y);

    if (mode === "collision") {

      const value = gridTool === "erase" ? 0 : 1;

      applyCollisionCells(cells, value);

      return;

    }

    if (mode === "fluids") {

      const value = gridTool === "erase" ? FLUID_NONE : paintFluidToId(fluidPaint);

      applyFluidCells(cells, value);

    }

  };



  const onFillTool = (x0: number, y0: number, x1: number, y1: number) => {
    if (mode === "tiles") {
      const p0 = placeAt(x0, y0);
      const p1 = placeAt(x1, y1);
      const tw = activeTileset?.tile_width ?? level.grid_size;
      const th = activeTileset?.tile_height ?? level.grid_size;

      if (gridTool === "erase") {
        update((prev) => {
          const { gridSize } = subGridDimensions(prev);
          const hits = placementsInRectInLevel(prev, tilesets, p0.x, p0.y, p1.x, p1.y, gridSize);
          if (hits.length === 0) return prev;
          return removePlacementsByGlobalIndices(prev, new Set(hits));
        });
      } else {
        const positions = snapTileAnchors(pixelFillPositions(p0.x, p0.y, p1.x, p1.y, tw, th));
        paintTilesAtPositions(positions);
        setStatus(`Fill: ${positions.length} tile(s)`);
      }

      return;
    }

    const { gridSize, cols, rows } = subGridDimensions(level);

    const cells = rectSubCells(x0, y0, x1, y1, gridSize, cols, rows);

    if (mode === "collision") {

      const value = gridTool === "erase" ? 0 : 1;

      applyCollisionCells(cells, value);

      return;

    }

    if (mode === "fluids") {

      const value = gridTool === "erase" ? FLUID_NONE : paintFluidToId(fluidPaint);

      applyFluidCells(cells, value);

    }

  };



  const onCanvasPointer = (

    worldX: number,

    worldY: number,

    button: number,

    strokeFrom?: { x: number; y: number },

  ) => {

    const { gridSize, cols, rows } = subGridDimensions(level);

    const { x: cx, y: cy } = worldToSubCell(worldX, worldY, gridSize);



    if (mode === "collision") {

      if (gridTool === "select" || gridTool === "erase") return;

      if (cx < 0 || cy < 0 || cx >= cols || cy >= rows) return;

      const value = 1;

      const cells = strokeFrom

        ? cellsOnWorldStroke(strokeFrom.x, strokeFrom.y, worldX, worldY, gridSize)

        : [{ x: cx, y: cy }];

      applyCollisionCells(cells, value);

      return;

    }



    if (mode === "fluids") {

      if (gridTool === "select" || gridTool === "erase") return;

      if (cx < 0 || cy < 0 || cx >= cols || cy >= rows) return;

      const value = paintFluidToId(fluidPaint);

      const cells = strokeFrom

        ? cellsOnWorldStroke(strokeFrom.x, strokeFrom.y, worldX, worldY, gridSize)

        : [{ x: cx, y: cy }];

      applyFluidCells(cells, value);

      return;

    }



    if (mode === "tiles") {
      if (gridTool === "paint" && button === 0) {
        const step = snapGrid ? gridSize : 8;
        const positions = strokeFrom
          ? snapTileAnchors(pixelLinePositions(strokeFrom.x, strokeFrom.y, worldX, worldY, step))
          : [placeAt(worldX, worldY)];
        paintTilesAtPositions(positions);
        return;
      }
    }



    if (mode === "objects") {

      if (button === 2 && objectTool === "select") {

        const hit = hitObject(level.objects, worldX, worldY, modelCatalog);

        if (hit >= 0) {

          update((prev) => ({

            ...prev,

            objects: prev.objects.filter((_, i) => i !== hit),

          }));

          setSelectedObject(-1);

        }

        return;

      }



      if (button !== 0) return;



      if (objectTool === "place-player" || objectTool === "place-block") {

        const pos = placeAt(worldX, worldY);

        const isPlayer = objectTool === "place-player";

        const pivot = isPlayer
          ? snapGrid
            ? playerPlacementPivot(pos.x, pos.y, subGridDimensions(level).gridSize)
            : { x: worldX, y: worldY }
          : pos;

        const object: LevelObject = {

          id: isPlayer ? "player" : `object_${level.objects.length}`,

          type: isPlayer ? "player" : "block",

          x: pivot.x,

          y: pivot.y,

          vel_x: 0,

          vel_y: 0,

        };

        if (!isPlayer) {

          object.width = 64;

          object.height = 64;

        }

        update((prev) => ({ ...prev, objects: [...prev.objects, object] }));

        setSelectedObject(level.objects.length);

        setObjectTool("select");

        return;

      }



      if (objectTool === "select") {

        setSelectedObject(hitObject(level.objects, worldX, worldY, modelCatalog));

      }

    }

  };



  const onObjectDrag = (index: number, worldX: number, worldY: number) => {

    const pos = placeAt(worldX, worldY);

    update((prev) => ({

      ...prev,

      objects: prev.objects.map((o, i) => (i === index ? { ...o, x: pos.x, y: pos.y } : o)),

    }));

  };



  const onPlacementsDrag = (indices: number[], anchorIndex: number, worldX: number, worldY: number) => {

    update((prev) => {

      const anchor = findPlacementRef(prev, anchorIndex)?.tile;

      if (!anchor) return prev;



      if (!placementDragSnapshotRef.current) {

        const snap = new Map<number, { x: number; y: number }>();

        for (const index of indices) {

          const tile = findPlacementRef(prev, index)?.tile;

          if (tile) snap.set(index, { x: tile.x, y: tile.y });

        }

        placementDragSnapshotRef.current = snap;

      }



      const origin = placementDragSnapshotRef.current.get(anchorIndex);

      if (!origin) return prev;

      const pos = placeAt(worldX, worldY);

      const deltaX = pos.x - origin.x;

      const deltaY = pos.y - origin.y;

      if (deltaX === 0 && deltaY === 0) return prev;



      const flat = flattenPlacements(prev);

      const snapshotTiles = flat.map((tile, i) => {

        const start = placementDragSnapshotRef.current?.get(i);

        if (!start) return tile;

        return { ...tile, x: start.x + deltaX, y: start.y + deltaY };

      });



      if (!canMovePlacements(snapshotTiles, indices, 0, 0, prev.width, prev.height)) {
        return prev;
      }

      const snapshot = placementDragSnapshotRef.current;
      if (!snapshot) {
        return prev;
      }

      return movePlacementsByGlobalIndicesFromSnapshot(prev, indices, snapshot, deltaX, deltaY);

    });

  };

  const onPlacementsDragEnd = () => {
    placementDragSnapshotRef.current = null;
  };

  const onGridCellsDrag = (deltaX: number, deltaY: number) => {
    if ((deltaX === 0 && deltaY === 0) || selectedGridCells.size === 0) return;

    if (!gridDragSnapshotRef.current) {
      gridDragSnapshotRef.current = {
        keys: new Set(selectedGridCells),
        collision:
          mode === "collision" ? level.collision.map((row) => [...row]) : undefined,
        fluids: mode === "fluids" ? level.fluids.map((row) => [...row]) : undefined,
      };
    }

    const snap = gridDragSnapshotRef.current;
    const { cols, rows } = subGridDimensions(level);
    const emptyValue = mode === "collision" ? 0 : FLUID_NONE;
    const baseGrid =
      mode === "collision" ? snap.collision! : mode === "fluids" ? snap.fluids! : null;
    if (!baseGrid) return;

    const moved = translateGridCellValues(
      baseGrid,
      snap.keys,
      deltaX,
      deltaY,
      cols,
      rows,
      emptyValue,
    );
    if (!moved) return;

    setSelectedGridCells(moved.movedKeys);
    update((prev) => {
      if (mode === "collision") {
        return { ...prev, collision: moved.grid };
      }
      return { ...prev, fluids: moved.grid };
    });
  };

  const onGridCellsDragEnd = () => {
    gridDragSnapshotRef.current = null;
  };

  const loadFile = async (file: File) => {

    try {

      const parsed = parseLevelJson(await file.text());

      replace(parsed, true);

      setSelectedObject(-1);

      setSelectedPlacement(-1);

      setSelectedPlacements(new Set());

      setSelectedGridCells(new Set());

      setMapRevision((v) => v + 1);

      setStatus(`Loaded ${file.name}`);

    } catch (error) {

      setStatus(error instanceof Error ? error.message : "Failed to load file");

    }

  };



  const resizeMap = (width: number, height: number) => {
    update((prev) => resizeLevel(prev, width, height));
    setMapRevision((v) => v + 1);
    setStatus(`Map resized to ${width}×${height} px`);
  };

  const changeGridSize = (gridSize: number) => {
    update((prev) => setGridSize(prev, gridSize));
    setMapRevision((v) => v + 1);
    setStatus(`Grid step: ${gridSize}px`);
  };



  const updateObject = (patch: Partial<LevelObject>) => {

    if (selectedObject < 0) return;

    update((prev) => ({

      ...prev,

      objects: prev.objects.map((o, i) => (i === selectedObject ? { ...o, ...patch } : o)),

    }));

  };



  return (

    <div className="editor-root">

      <MenuBar

        dirty={dirty}

        canUndo={canUndo}

        canRedo={canRedo}

        showJson={showJson}

        onUndo={() => {

          undo();

          setStatus("Undo");

        }}

        onRedo={() => {

          redo();

          setStatus("Redo");

        }}

        onOpen={() => fileInputRef.current?.click()}

        onNew={() => {

          replace(createBlankLevel());

          setSelectedObject(-1);

          setSelectedPlacement(-1);

          setSelectedPlacements(new Set());

          setSelectedGridCells(new Set());

          setMapRevision((v) => v + 1);

          setStatus("New blank level");

        }}

        onExport={exportLevel}

        onToggleJson={() => setShowJson((v) => !v)}

      />



      <div className={`editor-main ${showJson ? "with-json" : ""}`}>

        <ToolRail mode={mode} onModeChange={onModeChange} />



        <div className="editor-center">

          <OptionsBar

            mode={mode}

            gridTool={gridTool}

            objectTool={objectTool}

            fluidPaint={fluidPaint}

            onGridToolChange={onGridToolChange}

            onObjectToolChange={setObjectTool}

            onFluidPaintChange={setFluidPaint}

            tileLayers={level.tile_layers}

            activeTileLayerId={activeTileLayerId}

            onTileLayerChange={setActiveTileLayerId}

          />

          <LevelCanvas

            level={level}

            mode={mode}

            gridTool={gridTool}

            objectTool={objectTool}

            snapGrid={snapGrid}

            activeTilesetId={activeTilesetId}

            tilesets={tilesets}

            tilesetImages={tilesetImages}

            selectedObject={selectedObject}

            selectedPlacement={selectedPlacement}

            selectedPlacements={selectedPlacements}

            selectedGridCells={selectedGridCells}

            backgroundImage={backgroundImage}

            modelCatalog={modelCatalog}

            mapRevision={mapRevision}

            onPointer={onCanvasPointer}

            onStrokeBegin={beginStroke}

            onStrokeEnd={endStroke}

            onObjectDrag={onObjectDrag}

            onPlacementsDrag={onPlacementsDrag}

            onPlacementsDragEnd={onPlacementsDragEnd}

            onGridCellsDrag={onGridCellsDrag}

            onGridCellsDragEnd={onGridCellsDragEnd}

            onClearGridSelection={clearGridSelection}

            onGridCellPick={onGridCellPick}

            onRectSelect={onRectSelect}

            onRectErase={onRectErase}

            onLineTool={onLineTool}

            onFillTool={onFillTool}

            onTilePick={onTilePick}

          />

          {mode === "tiles" && (

            <TilesetDock

              tilesets={tilesets}

              tilesetImages={tilesetImages}

              activeTilesetId={activeTilesetId}

              selectedTileId={selectedTileId}

              paintCollision={paintCollision}

              paintFluid={paintFluid}

              onTilesetChange={setActiveTilesetId}

              onOpenPicker={() => setTilePickerOpen(true)}

              onPaintCollisionChange={setPaintCollision}

              onPaintFluidChange={setPaintFluid}

            />

          )}

        </div>



        <Inspector

          level={level}

          mode={mode}

          selected={selected}

          selectedPlacement={selectedPlacement}

          selectedPlacementCount={

            selectedPlacements.size > 0 ? selectedPlacements.size : selectedPlacement >= 0 ? 1 : 0

          }

          selectedGridCellCount={selectedGridCells.size}

          tilesets={tilesets}

          tilesetImages={tilesetImages}

          backgroundAssets={pack.backgrounds}

          soundAssets={pack.music}

          modelCatalog={modelCatalog}

          snapGrid={snapGrid}

          onLevelPatch={(patch) => update((prev) => ({ ...prev, ...patch }))}

          onResizeMap={resizeMap}

          onObjectPatch={updateObject}

          onUpdatePlacement={updatePlacement}

          onDeleteObject={deleteSelectedObject}

          onDeletePlacement={deleteSelectedPlacement}

          onDeleteGridSelection={deleteSelectedGridCells}

          onClearGridSelection={clearGridSelection}

          onAddFrame={openFramePicker}

          onSnapGridChange={setSnapGrid}
          onGridSizeChange={changeGridSize}

        />



        {showJson && <JsonPreview level={level} />}

      </div>



      <StatusBar text={status} />



      <TilesetPickerModal

        open={tilePickerOpen}

        tileset={tilesets.get(activeTilesetId) ?? null}

        image={tilesetImages.get(activeTilesetId) ?? null}

        selectedTileId={selectedTileId}

        onSelect={setSelectedTileId}

        onClose={() => setTilePickerOpen(false)}

      />



      <TilesetPickerModal

        open={framePickerOpen}

        tileset={tilesets.get(framePickerTilesetId) ?? null}

        image={tilesetImages.get(framePickerTilesetId) ?? null}

        selectedTileId={0}

        tilesets={tilesets}

        tilesetImages={tilesetImages}

        activeTilesetId={framePickerTilesetId}

        onTilesetChange={setFramePickerTilesetId}

        onSelectFrame={addFrameToPlacement}

        onClose={() => setFramePickerOpen(false)}

        title="Pick animation frame"

      />



      <input

        ref={fileInputRef}

        type="file"

        accept=".json,.level,application/json"

        hidden

        onChange={(e) => {

          const file = e.target.files?.[0];

          if (file) void loadFile(file);

          e.target.value = "";

        }}

      />

    </div>

  );

}


