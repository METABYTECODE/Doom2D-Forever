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
import { FLUID_NONE } from "./types/level";
import { paintFluidCells } from "./lib/level-fluids";
import {
  createBlankLevel,
  downloadLevel,
  parseLevelJson,
  resizeLevel,
} from "./lib/level-io";
import { applyTilePaintLayers, paintFluidToId } from "./lib/tile-layers";
import { canPlaceTile, findPlacementAt } from "./lib/tile-placements";
import { validateLevel } from "./lib/level-validation";
import {
  DEFAULT_PACK_ID,
  loadPackImage,
  loadResourcePack,
  loadTilesetImage,
} from "./lib/resource-pack";
import type {
  CollisionTool,
  EditorMode,
  FluidTool,
  LevelObject,
  ObjectTool,
  PaintFluidOption,
  PlacedTile,
  TileTool,
} from "./types/level";
import { DEFAULT_FRAME_MS } from "./types/level";
import type { TilesetDef } from "./types/tileset";

export function App() {
  const { level, dirty, replace, update, beginStroke, endStroke, undo, redo, markSaved, canUndo, canRedo } =
    useLevelHistory(createBlankLevel());
  const [mode, setMode] = useState<EditorMode>("tiles");
  const [tileTool, setTileTool] = useState<TileTool>("paint");
  const [collisionTool, setCollisionTool] = useState<CollisionTool>("paint");
  const [fluidTool, setFluidTool] = useState<FluidTool>("water");
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
  const [snapGrid, setSnapGrid] = useState(true);
  const [showJson, setShowJson] = useState(false);
  const [status, setStatus] = useState("Loading…");
  const [mapRevision, setMapRevision] = useState(0);
  const [tilePickerOpen, setTilePickerOpen] = useState(false);
  const [framePickerOpen, setFramePickerOpen] = useState(false);
  const [framePickerTilesetId, setFramePickerTilesetId] = useState("");
  const fileInputRef = useRef<HTMLInputElement>(null);

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
    if (selectedPlacement < 0) return;
    update((prev) => ({
      ...prev,
      tiles: prev.tiles.filter((_, i) => i !== selectedPlacement),
    }));
    setSelectedPlacement(-1);
    setStatus("Tile deleted");
  }, [selectedPlacement, update]);

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
        if (selectedPlacement >= 0 && mode === "tiles") deleteSelectedPlacement();
        else if (selectedObject >= 0) deleteSelectedObject();
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
    mode,
    redo,
    selectedObject,
    selectedPlacement,
    undo,
  ]);

  const selected = useMemo(
    () => (selectedObject >= 0 ? level.objects[selectedObject] : null),
    [level.objects, selectedObject],
  );

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
      const value = collisionTool === "paint" ? 1 : 0;
      const cells = strokeBrushCells(
        strokeFrom,
        tx,
        ty,
        brushSize,
        level.width,
        level.height,
      );
      update((prev) => {
        const collision = prev.collision.map((row) => [...row]);
        if (!paintCollisionCells(collision, cells, value, prev.width, prev.height)) return prev;
        return { ...prev, collision };
      });
      return;
    }

    if (mode === "fluids") {
      if (tx < 0 || ty < 0 || tx >= level.width || ty >= level.height) return;
      const value = fluidTool === "erase" ? FLUID_NONE : paintFluidToId(fluidTool);
      const cells = strokeBrushCells(
        strokeFrom,
        tx,
        ty,
        brushSize,
        level.width,
        level.height,
      );
      update((prev) => {
        const fluids = prev.fluids.map((row) => [...row]);
        if (!paintFluidCells(fluids, cells, value, prev.width, prev.height)) return prev;
        return { ...prev, fluids };
      });
      return;
    }

    if (mode === "tiles") {
      if (tileTool === "paint" && button === 0) {
        const cells = strokeFrom
          ? gridLineCells(strokeFrom.tx, strokeFrom.ty, tx, ty)
          : [{ x: tx, y: ty }];
        update((prev) => {
          const tiles = [...prev.tiles];
          const collision = prev.collision.map((row) => [...row]);
          const fluids = prev.fluids.map((row) => [...row]);
          let changed = false;
          for (const { x, y } of cells) {
            if (x < 0 || y < 0 || x >= prev.width || y >= prev.height) continue;
            if (
              !canPlaceTile(
                tiles,
                tilesets,
                activeTilesetId,
                selectedTileId,
                x,
                y,
                prev.width,
                prev.height,
                -1,
                prev.grid_size,
              )
            ) {
              continue;
            }
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
        return;
      }

      if (tileTool === "erase" && button === 0) {
        const cells = strokeFrom
          ? gridLineCells(strokeFrom.tx, strokeFrom.ty, tx, ty)
          : [{ x: tx, y: ty }];
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
          }
          return { ...prev, tiles: nextTiles };
        });
        return;
      }

      if (tileTool === "select" && button === 0) {
        const hit = findPlacementAt(level.tiles, tilesets, tx, ty, level.grid_size);
        setSelectedPlacement(hit);
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

  const onPlacementDrag = (index: number, cellX: number, cellY: number) => {
    update((prev) => {
      const current = prev.tiles[index];
      if (!current) return prev;
      if (
        !canPlaceTile(
          prev.tiles,
          tilesets,
          current.tileset,
          current.id,
          cellX,
          cellY,
          prev.width,
          prev.height,
          index,
          prev.grid_size,
        )
      ) {
        return prev;
      }
      return {
        ...prev,
        tiles: prev.tiles.map((tile, i) => (i === index ? { ...tile, x: cellX, y: cellY } : tile)),
      };
    });
  };

  const loadFile = async (file: File) => {
    try {
      const parsed = parseLevelJson(await file.text());
      replace(parsed, true);
      setSelectedObject(-1);
      setSelectedPlacement(-1);
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
          setMapRevision((v) => v + 1);
          setStatus("New blank level");
        }}
        onExport={exportLevel}
        onToggleJson={() => setShowJson((v) => !v)}
      />

      <div className={`editor-main ${showJson ? "with-json" : ""}`}>
        <ToolRail
          mode={mode}
          tileTool={tileTool}
          collisionTool={collisionTool}
          fluidTool={fluidTool}
          objectTool={objectTool}
          onModeChange={setMode}
          onTileToolChange={setTileTool}
          onCollisionToolChange={setCollisionTool}
          onFluidToolChange={setFluidTool}
          onObjectToolChange={setObjectTool}
        />

        <div className="editor-center">
          <OptionsBar mode={mode} brushSize={brushSize} onBrushSizeChange={setBrushSize} />
          <LevelCanvas
            level={level}
            mode={mode}
            tileTool={tileTool}
            collisionTool={collisionTool}
            fluidTool={fluidTool}
            objectTool={objectTool}
            brushSize={brushSize}
            activeTilesetId={activeTilesetId}
            tilesets={tilesets}
            tilesetImages={tilesetImages}
            selectedObject={selectedObject}
            selectedPlacement={selectedPlacement}
            backgroundImage={backgroundImage}
            mapRevision={mapRevision}
            onPointer={onCanvasPointer}
            onStrokeBegin={beginStroke}
            onStrokeEnd={endStroke}
            onObjectDrag={onObjectDrag}
            onPlacementDrag={onPlacementDrag}
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
