import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { JsonPreview } from "./components/JsonPreview";
import {
  Inspector,
  MenuBar,
  StatusBar,
  TilesetDock,
  ToolRail,
} from "./components/EditorLayout";
import { LevelCanvas } from "./components/LevelCanvas";
import { TilesetPickerModal } from "./components/TilesetPickerModal";
import { useLevelHistory } from "./hooks/useLevelHistory";
import { hitObject, snapPoint, tileAt } from "./lib/geometry";
import { worldCellSize } from "./lib/level-collision";
import {
  createBlankLevel,
  downloadLevel,
  parseLevelJson,
  resizeLevel,
} from "./lib/level-io";
import { canPlaceTile, findPlacementAt } from "./lib/tile-placements";
import { validateLevel } from "./lib/level-validation";
import { loadAllTilesets, loadTilesetImage } from "./lib/tileset-io";
import type {
  CollisionTool,
  EditorMode,
  LevelObject,
  ObjectTool,
  PlacedTile,
  TileTool,
} from "./types/level";
import type { TilesetDef } from "./types/tileset";

export function App() {
  const { level, dirty, replace, update, undo, redo, markSaved, canUndo, canRedo } =
    useLevelHistory(createBlankLevel());
  const [mode, setMode] = useState<EditorMode>("tiles");
  const [tileTool, setTileTool] = useState<TileTool>("paint");
  const [collisionTool, setCollisionTool] = useState<CollisionTool>("paint");
  const [objectTool, setObjectTool] = useState<ObjectTool>("select");
  const [tilesets, setTilesets] = useState<Map<string, TilesetDef>>(() => loadAllTilesets());
  const [tilesetImages, setTilesetImages] = useState<Map<string, HTMLImageElement>>(new Map());
  const [activeTilesetId, setActiveTilesetId] = useState("");
  const [selectedTileId, setSelectedTileId] = useState(0);
  const [selectedObject, setSelectedObject] = useState(-1);
  const [selectedPlacement, setSelectedPlacement] = useState(-1);
  const [snapGrid, setSnapGrid] = useState(true);
  const [showJson, setShowJson] = useState(false);
  const [status, setStatus] = useState("Loading…");
  const [mapRevision, setMapRevision] = useState(0);
  const [tilePickerOpen, setTilePickerOpen] = useState(false);
  const fileInputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    const loaded = loadAllTilesets();
    setTilesets(loaded);
    const first = loaded.keys().next().value as string | undefined;
    if (first) setActiveTilesetId(first);

    void (async () => {
      const images = new Map<string, HTMLImageElement>();
      await Promise.all(
        [...loaded.values()].map(async (tileset) => {
          try {
            images.set(tileset.id, await loadTilesetImage(tileset));
          } catch {
            /* missing texture */
          }
        }),
      );
      setTilesetImages(images);
      if (images.size === 0 && loaded.size > 0) {
        setStatus("Tilesets loaded — add matching PNG next to each JSON in src/tilesets/");
      }
    })();
  }, []);

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

  const onCanvasPointer = (worldX: number, worldY: number, button: number) => {
    const cell = worldCellSize(level);
    const { x: tx, y: ty } = tileAt(worldX, worldY, cell);

    if (mode === "collision") {
      if (tx < 0 || ty < 0 || tx >= level.width || ty >= level.height) return;
      const value = collisionTool === "paint" ? 1 : 0;
      update((prev) => {
        const collision = prev.collision.map((row) => [...row]);
        if ((collision[ty]?.[tx] ?? 0) === value) return prev;
        collision[ty][tx] = value;
        return { ...prev, collision };
      });
      return;
    }

    if (mode === "tiles") {
      if (tileTool === "paint" && button === 0) {
        if (tx < 0 || ty < 0 || tx >= level.width || ty >= level.height) return;
        if (!canPlaceTile(level.tiles, tilesets, activeTilesetId, selectedTileId, tx, ty, level.width, level.height)) {
          return;
        }
        const placement: PlacedTile = {
          tileset: activeTilesetId,
          id: selectedTileId,
          x: tx,
          y: ty,
        };
        update((prev) => ({ ...prev, tiles: [...prev.tiles, placement] }));
        return;
      }

      if (tileTool === "erase" && button === 0) {
        const hit = findPlacementAt(level.tiles, tilesets, tx, ty);
        if (hit < 0) return;
        update((prev) => ({ ...prev, tiles: prev.tiles.filter((_, i) => i !== hit) }));
        if (selectedPlacement === hit) setSelectedPlacement(-1);
        return;
      }

      if (tileTool === "select" && button === 0) {
        const hit = findPlacementAt(level.tiles, tilesets, tx, ty);
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
          objectTool={objectTool}
          onModeChange={setMode}
          onTileToolChange={setTileTool}
          onCollisionToolChange={setCollisionTool}
          onObjectToolChange={setObjectTool}
        />

        <div className="editor-center">
          <LevelCanvas
            level={level}
            mode={mode}
            tileTool={tileTool}
            collisionTool={collisionTool}
            objectTool={objectTool}
            tilesets={tilesets}
            tilesetImages={tilesetImages}
            selectedObject={selectedObject}
            selectedPlacement={selectedPlacement}
            mapRevision={mapRevision}
            onPointer={onCanvasPointer}
            onObjectDrag={onObjectDrag}
            onPlacementDrag={onPlacementDrag}
          />
          {mode === "tiles" && (
            <TilesetDock
              tilesets={tilesets}
              tilesetImages={tilesetImages}
              activeTilesetId={activeTilesetId}
              selectedTileId={selectedTileId}
              onTilesetChange={setActiveTilesetId}
              onOpenPicker={() => setTilePickerOpen(true)}
            />
          )}
        </div>

        <Inspector
          level={level}
          mode={mode}
          selected={selected}
          selectedPlacement={selectedPlacement}
          snapGrid={snapGrid}
          onLevelPatch={(patch) => update((prev) => ({ ...prev, ...patch }))}
          onResizeMap={resizeMap}
          onObjectPatch={updateObject}
          onDeleteObject={deleteSelectedObject}
          onDeletePlacement={deleteSelectedPlacement}
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
