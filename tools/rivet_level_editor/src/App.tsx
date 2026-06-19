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
import { hitObject, snapPoint, tileAt } from "./lib/geometry";
import { strokeBrushCells } from "./lib/brush";
import { gridLineCells, paintCollisionCells, worldCellSize } from "./lib/level-collision";
import { rectCells } from "./lib/grid-rect";
import { FLUID_NONE } from "./types/level";
import { paintFluidCells } from "./lib/level-fluids";
import {
  createBlankLevel,
  downloadLevel,
  parseLevelJson,
  resizeLevel,
} from "./lib/level-io";
import { applyTilePaintLayers, paintFluidToId } from "./lib/tile-layers";
import {
  findPlacementAt,
  removeOverlappingPlacements,
} from "./lib/tile-placements";
import {
  canMovePlacements,
  placementsInRect,
} from "./lib/tile-selection";
import { tileCellSpan } from "./lib/tile-math";
import { validateLevel } from "./lib/level-validation";
import {
  DEFAULT_PACK_ID,
  loadPackImage,
  loadResourcePack,
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
import type { TilesetDef } from "./types/tileset";

export function App() {
  const { level, dirty, replace, update, beginStroke, endStroke, undo, redo, markSaved, canUndo, canRedo } =
    useLevelHistory(createBlankLevel());
  const [mode, setMode] = useState<EditorMode>("tiles");
  const [gridTool, setGridTool] = useState<GridTool>("select");
  const [fluidPaint, setFluidPaint] = useState<FluidPaint>("water");
  const [objectTool, setObjectTool] = useState<ObjectTool>("select");
  const [brushSize, setBrushSize] = useState(1);
  const [paintCollision, setPaintCollision] = useState(false);
  const [paintFluid, setPaintFluid] = useState<PaintFluidOption>("none");
  const pack = useMemo(
    () => loadResourcePack(level.resource_pack || DEFAULT_PACK_ID),
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
  const [snapGrid, setSnapGrid] = useState(true);
  const [showJson, setShowJson] = useState(false);
  const [status, setStatus] = useState("Loading…");
  const [mapRevision, setMapRevision] = useState(0);
  const [tilePickerOpen, setTilePickerOpen] = useState(false);
  const [framePickerOpen, setFramePickerOpen] = useState(false);
  const [framePickerTilesetId, setFramePickerTilesetId] = useState("");
  const fileInputRef = useRef<HTMLInputElement>(null);
  const placementDragSnapshotRef = useRef<Map<number, { x: number; y: number }> | null>(null);

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
    void fetch("/sample.level.json")
      .then((r) => (r.ok ? r.text() : Promise.reject()))
      .then((text) => {
        replace(parseLevelJson(text), true);
        setMapRevision((v) => v + 1);
        setStatus("Loaded sample.level.json");
      })
      .catch(() => setStatus("New blank level"));
  }, [replace]);

  const exportLevel = useCallback(() => {
    const errors = validateLevel(level);
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
      update((prev) => ({
        ...prev,
        tiles: prev.tiles.map((tile, i) => (i === index ? { ...tile, ...patch } : tile)),
      }));
    },
    [update],
  );

  const openFramePicker = useCallback(() => {
    if (selectedPlacement < 0) return;
    const placement = level.tiles[selectedPlacement];
    if (!placement) return;
    setFramePickerTilesetId(placement.tileset || activeTilesetId);
    setFramePickerOpen(true);
  }, [activeTilesetId, level.tiles, selectedPlacement]);

  const addFrameToPlacement = useCallback(
    (tilesetId: string, tileId: number) => {
      if (selectedPlacement < 0) return;
      update((prev) => {
        const current = prev.tiles[selectedPlacement];
        if (!current) return prev;
        const frames = current.frames?.length
          ? [...current.frames]
          : [{ tileset: current.tileset, id: current.id }];
        frames.push({ tileset: tilesetId, id: tileId });
        return {
          ...prev,
          tiles: prev.tiles.map((tile, i) =>
            i === selectedPlacement
              ? { ...tile, frames, frame_ms: tile.frame_ms ?? DEFAULT_FRAME_MS }
              : tile,
          ),
        };
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
    update((prev) => ({
      ...prev,
      tiles: prev.tiles.filter((_, i) => !toRemove.has(i)),
    }));
    setSelectedPlacement(-1);
    setSelectedPlacements(new Set());
    setStatus(`Deleted ${toRemove.size} tile(s)`);
  }, [selectedPlacement, selectedPlacements, update]);

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
        } else if (selectedObject >= 0) deleteSelectedObject();
        return;
      }
      if (e.ctrlKey && e.key.toLowerCase() === "a" && mode === "tiles") {
        e.preventDefault();
        const all = new Set(level.tiles.map((_, i) => i));
        setSelectedPlacements(all);
        setSelectedPlacement(level.tiles.length > 0 ? 0 : -1);
        setStatus(`Selected ${all.size} tile(s)`);
        return;
      }
      if (e.key === "t") setMode("tiles");
      if (e.key === "c") setMode("collision");
      if (e.key === "f") setMode("fluids");
      if (e.key === "o") setMode("objects");
    };
    window.addEventListener("keydown", onKeyDown);
    return () => window.removeEventListener("keydown", onKeyDown);
  }, [
    deleteSelectedObject,
    deleteSelectedPlacement,
    exportLevel,
    level.tiles,
    mode,
    redo,
    selectedObject,
    selectedPlacement,
    selectedPlacements.size,
    undo,
  ]);

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
    const hits = placementsInRect(level.tiles, tilesets, x0, y0, x1, y1, level.grid_size);
    if (additive) {
      const next = new Set(selectedPlacements);
      for (const index of hits) next.add(index);
      syncPrimarySelection(next);
    } else {
      syncPrimarySelection(new Set(hits));
    }
    setStatus(`Selected ${hits.length} tile(s)`);
  };

  const paintTilesAtCells = (cells: Array<{ x: number; y: number }>) => {
    update((prev) => {
      let tiles = [...prev.tiles];
      const collision = prev.collision.map((row) => [...row]);
      const fluids = prev.fluids.map((row) => [...row]);
      let changed = false;

      for (const { x, y } of cells) {
        if (x < 0 || y < 0 || x >= prev.width || y >= prev.height) continue;
        const ts = tilesets.get(activeTilesetId);
        const span = ts ? tileCellSpan(ts, prev.grid_size) : { w: 1, h: 1 };
        if (x + span.w > prev.width || y + span.h > prev.height) continue;

        const cleared = removeOverlappingPlacements(
          tiles,
          tilesets,
          x,
          y,
          span.w,
          span.h,
          new Set(),
          prev.grid_size,
        );
        if (cleared.tiles.length !== tiles.length) changed = true;
        tiles = cleared.tiles;
        tiles.push({
          tileset: activeTilesetId,
          id: selectedTileId,
          x,
          y,
        });
        changed = true;

        if (paintCollision || paintFluid !== "none") {
          changed =
            applyTilePaintLayers(
              collision,
              fluids,
              x,
              y,
              tilesets,
              activeTilesetId,
              paintCollision,
              paintFluid,
              prev.width,
              prev.height,
              prev.grid_size,
            ) || changed;
        }
      }

      if (!changed) return prev;
      return { ...prev, tiles, collision, fluids };
    });
  };

  const eraseTilesAtCells = (cells: Array<{ x: number; y: number }>) => {
    update((prev) => {
      const toRemove = new Set<number>();
      for (const { x, y } of cells) {
        const hit = findPlacementAt(prev.tiles, tilesets, x, y, prev.grid_size);
        if (hit >= 0) toRemove.add(hit);
      }
      if (toRemove.size === 0) return prev;
      const nextTiles = prev.tiles.filter((_, i) => !toRemove.has(i));
      if (selectedPlacement >= 0 && toRemove.has(selectedPlacement)) {
        setSelectedPlacement(-1);
      setSelectedPlacements(new Set());
      }
      setSelectedPlacements((current) => {
        const next = new Set<number>();
        for (const index of current) {
          if (!toRemove.has(index)) next.add(index);
        }
        return next;
      });
      return { ...prev, tiles: nextTiles };
    });
  };

  const applyCollisionCells = (
    cells: Array<{ x: number; y: number }>,
    value: number,
  ) => {
    update((prev) => {
      const collision = prev.collision.map((row) => [...row]);
      if (!paintCollisionCells(collision, cells, value, prev.width, prev.height)) return prev;
      return { ...prev, collision };
    });
  };

  const applyFluidCells = (cells: Array<{ x: number; y: number }>, value: number) => {
    update((prev) => {
      const fluids = prev.fluids.map((row) => [...row]);
      if (!paintFluidCells(fluids, cells, value, prev.width, prev.height)) return prev;
      return { ...prev, fluids };
    });
  };

  const onLineTool = (x0: number, y0: number, x1: number, y1: number) => {
    const cells = gridLineCells(x0, y0, x1, y1);
    if (mode === "tiles") {
      paintTilesAtCells(cells);
      return;
    }
    if (mode === "collision") {
      const value = gridTool === "erase" ? 0 : 1;
      const expanded = cells.flatMap((cell) =>
        strokeBrushCells(undefined, cell.x, cell.y, brushSize, level.width, level.height),
      );
      applyCollisionCells(expanded, value);
      return;
    }
    if (mode === "fluids") {
      const value = gridTool === "erase" ? FLUID_NONE : paintFluidToId(fluidPaint);
      const expanded = cells.flatMap((cell) =>
        strokeBrushCells(undefined, cell.x, cell.y, brushSize, level.width, level.height),
      );
      applyFluidCells(expanded, value);
    }
  };

  const onFillTool = (x0: number, y0: number, x1: number, y1: number) => {
    const cells = rectCells(x0, y0, x1, y1, level.width, level.height);
    if (mode === "tiles") {
      if (gridTool === "erase") eraseTilesAtCells(cells);
      else paintTilesAtCells(cells);
      return;
    }
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

  const placeAt = (worldX: number, worldY: number) => {
    const cell = worldCellSize(level);
    return snapGrid ? snapPoint(worldX, worldY, cell) : { x: worldX, y: worldY };
  };

  const onCanvasPointer = (
    worldX: number,
    worldY: number,
    button: number,
    strokeFrom?: { tx: number; ty: number },
  ) => {
    const cell = worldCellSize(level);
    const { x: tx, y: ty } = tileAt(worldX, worldY, cell);

    if (mode === "collision") {
      if (tx < 0 || ty < 0 || tx >= level.width || ty >= level.height) return;
      const value = gridTool === "erase" ? 0 : 1;
      const cells = strokeBrushCells(
        strokeFrom,
        tx,
        ty,
        brushSize,
        level.width,
        level.height,
      );
      applyCollisionCells(cells, value);
      return;
    }

    if (mode === "fluids") {
      if (tx < 0 || ty < 0 || tx >= level.width || ty >= level.height) return;
      const value = gridTool === "erase" ? FLUID_NONE : paintFluidToId(fluidPaint);
      const cells = strokeBrushCells(
        strokeFrom,
        tx,
        ty,
        brushSize,
        level.width,
        level.height,
      );
      applyFluidCells(cells, value);
      return;
    }

    if (mode === "tiles") {
      if (gridTool === "paint" && button === 0) {
        const cells = strokeFrom
          ? gridLineCells(strokeFrom.tx, strokeFrom.ty, tx, ty)
          : [{ x: tx, y: ty }];
        paintTilesAtCells(cells);
        return;
      }

      if (gridTool === "erase" && button === 0) {
        const cells = strokeFrom
          ? gridLineCells(strokeFrom.tx, strokeFrom.ty, tx, ty)
          : [{ x: tx, y: ty }];
        eraseTilesAtCells(cells);
        return;
      }
    }

    if (mode === "objects") {
      if (button === 2 && objectTool === "select") {
        const hit = hitObject(level.objects, worldX, worldY);
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
        const object: LevelObject = {
          id: isPlayer ? "player" : `object_${level.objects.length}`,
          type: isPlayer ? "player" : "block",
          x: pos.x,
          y: pos.y,
          width: isPlayer ? 40 : 64,
          height: isPlayer ? 40 : 64,
          vel_x: 0,
          vel_y: 0,
        };
        update((prev) => ({ ...prev, objects: [...prev.objects, object] }));
        setSelectedObject(level.objects.length);
        setObjectTool("select");
        return;
      }

      if (objectTool === "select") {
        setSelectedObject(hitObject(level.objects, worldX, worldY));
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

  const onPlacementsDrag = (indices: number[], anchorIndex: number, cellX: number, cellY: number) => {
    update((prev) => {
      const anchor = prev.tiles[anchorIndex];
      if (!anchor) return prev;

      if (!placementDragSnapshotRef.current) {
        const snap = new Map<number, { x: number; y: number }>();
        for (const index of indices) {
          const tile = prev.tiles[index];
          if (tile) snap.set(index, { x: tile.x, y: tile.y });
        }
        placementDragSnapshotRef.current = snap;
      }

      const origin = placementDragSnapshotRef.current.get(anchorIndex);
      if (!origin) return prev;
      const deltaX = cellX - origin.x;
      const deltaY = cellY - origin.y;
      if (deltaX === 0 && deltaY === 0) return prev;

      const snapshotTiles = prev.tiles.map((tile, i) => {
        const start = placementDragSnapshotRef.current?.get(i);
        if (!start) return tile;
        return { ...tile, x: start.x + deltaX, y: start.y + deltaY };
      });

      if (
        !canMovePlacements(
          snapshotTiles,
          tilesets,
          indices,
          0,
          0,
          prev.width,
          prev.height,
          prev.grid_size,
        )
      ) {
        return prev;
      }

      return { ...prev, tiles: snapshotTiles };
    });
  };

  const onPlacementsDragEnd = () => {
    placementDragSnapshotRef.current = null;
  };

  const loadFile = async (file: File) => {
    try {
      const parsed = parseLevelJson(await file.text());
      replace(parsed, true);
      setSelectedObject(-1);
      setSelectedPlacement(-1);
      setSelectedPlacements(new Set());
      setMapRevision((v) => v + 1);
      setStatus(`Loaded ${file.name}`);
    } catch (error) {
      setStatus(error instanceof Error ? error.message : "Failed to load file");
    }
  };

  const resizeMap = (width: number, height: number) => {
    update((prev) => resizeLevel(prev, width, height));
    setMapRevision((v) => v + 1);
    setStatus(`Map resized to ${width}×${height} cells`);
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
          setMapRevision((v) => v + 1);
          setStatus("New blank level");
        }}
        onExport={exportLevel}
        onToggleJson={() => setShowJson((v) => !v)}
      />

      <div className={`editor-main ${showJson ? "with-json" : ""}`}>
        <ToolRail mode={mode} onModeChange={setMode} />

        <div className="editor-center">
          <OptionsBar
            mode={mode}
            gridTool={gridTool}
            objectTool={objectTool}
            fluidPaint={fluidPaint}
            brushSize={brushSize}
            onGridToolChange={setGridTool}
            onObjectToolChange={setObjectTool}
            onFluidPaintChange={setFluidPaint}
            onBrushSizeChange={setBrushSize}
          />
          <LevelCanvas
            level={level}
            mode={mode}
            gridTool={gridTool}
            objectTool={objectTool}
            brushSize={brushSize}
            activeTilesetId={activeTilesetId}
            tilesets={tilesets}
            tilesetImages={tilesetImages}
            selectedObject={selectedObject}
            selectedPlacement={selectedPlacement}
            selectedPlacements={selectedPlacements}
            backgroundImage={backgroundImage}
            mapRevision={mapRevision}
            onPointer={onCanvasPointer}
            onStrokeBegin={beginStroke}
            onStrokeEnd={endStroke}
            onObjectDrag={onObjectDrag}
            onPlacementsDrag={onPlacementsDrag}
            onPlacementsDragEnd={onPlacementsDragEnd}
            onMarqueeSelect={onMarqueeSelect}
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
          tilesets={tilesets}
          tilesetImages={tilesetImages}
          backgroundAssets={pack.backgrounds}
          soundAssets={pack.music}
          snapGrid={snapGrid}
          onLevelPatch={(patch) => update((prev) => ({ ...prev, ...patch }))}
          onResizeMap={resizeMap}
          onObjectPatch={updateObject}
          onUpdatePlacement={updatePlacement}
          onDeleteObject={deleteSelectedObject}
          onDeletePlacement={deleteSelectedPlacement}
          onAddFrame={openFramePicker}
          onSnapGridChange={setSnapGrid}
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
