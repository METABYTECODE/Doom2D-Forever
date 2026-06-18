import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { JsonPreview } from "./components/JsonPreview";
import { LevelCanvas } from "./components/LevelCanvas";
import { Sidebar } from "./components/Sidebar";
import { useLevelHistory } from "./hooks/useLevelHistory";
import { hitObject, snapPoint, tileAt } from "./lib/geometry";
import {
  createBlankLevel,
  downloadLevel,
  parseLevelJson,
  resizeTiles,
} from "./lib/level-io";
import { validateLevel } from "./lib/level-validation";
import type { EditorTool, LevelObject } from "./types/level";

const TOOL_KEYS: Record<string, EditorTool> = {
  "1": "paint",
  "2": "erase",
  "3": "place-player",
  "4": "place-block",
  "5": "select",
};

export function App() {
  const { level, dirty, replace, update, undo, redo, markSaved, canUndo, canRedo } =
    useLevelHistory(createBlankLevel());
  const [tool, setTool] = useState<EditorTool>("paint");
  const [paintTile, setPaintTile] = useState(1);
  const [selectedObject, setSelectedObject] = useState(-1);
  const [snapGrid, setSnapGrid] = useState(true);
  const [showJson, setShowJson] = useState(false);
  const [status, setStatus] = useState("Loading sample…");
  const [mapRevision, setMapRevision] = useState(0);
  const fileInputRef = useRef<HTMLInputElement>(null);

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

  const deleteSelected = useCallback(() => {
    if (selectedObject < 0) return;
    update((prev) => ({
      ...prev,
      objects: prev.objects.filter((_, i) => i !== selectedObject),
    }));
    setSelectedObject(-1);
    setStatus("Object deleted");
  }, [selectedObject, update]);

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
      if (e.key === "Delete" && selectedObject >= 0) {
        deleteSelected();
        return;
      }
      const nextTool = TOOL_KEYS[e.key];
      if (nextTool) setTool(nextTool);
    };
    window.addEventListener("keydown", onKeyDown);
    return () => window.removeEventListener("keydown", onKeyDown);
  }, [deleteSelected, exportLevel, redo, selectedObject, undo]);

  const selected = useMemo(
    () => (selectedObject >= 0 ? level.objects[selectedObject] : null),
    [level.objects, selectedObject],
  );

  const placeAt = (worldX: number, worldY: number) =>
    snapGrid ? snapPoint(worldX, worldY, level.tile_size) : { x: worldX, y: worldY };

  const onCanvasPointer = (worldX: number, worldY: number, button: number) => {
    const { x: tx, y: ty } = tileAt(worldX, worldY, level.tile_size);

    if (tool === "paint" && button === 0) {
      if (tx < 0 || ty < 0 || tx >= level.width || ty >= level.height) return;
      update((prev) => {
        const tiles = prev.tiles.map((row) => [...row]);
        if ((tiles[ty]?.[tx] ?? 0) === paintTile) return prev;
        tiles[ty][tx] = paintTile;
        return { ...prev, tiles };
      });
      return;
    }

    if (tool === "erase" && button === 0) {
      if (tx < 0 || ty < 0 || tx >= level.width || ty >= level.height) return;
      update((prev) => {
        const tiles = prev.tiles.map((row) => [...row]);
        if ((tiles[ty]?.[tx] ?? 0) === 0) return prev;
        tiles[ty][tx] = 0;
        return { ...prev, tiles };
      });
      return;
    }

    if (button === 2 && tool === "select") {
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

    if (tool === "place-player" || tool === "place-block") {
      const pos = placeAt(worldX, worldY);
      const isPlayer = tool === "place-player";
      const object: LevelObject = {
        id: isPlayer ? "player" : `object_${level.objects.length}`,
        type: isPlayer ? "player" : "block",
        x: pos.x,
        y: pos.y,
        width: isPlayer ? 40 : 64,
        height: isPlayer ? 40 : 64,
        vel_x: isPlayer ? 180 : 0,
        vel_y: isPlayer ? 140 : 0,
      };
      update((prev) => ({ ...prev, objects: [...prev.objects, object] }));
      setSelectedObject(level.objects.length);
      setTool("select");
      return;
    }

    if (tool === "select") {
      setSelectedObject(hitObject(level.objects, worldX, worldY));
    }
  };

  const onObjectDrag = (index: number, worldX: number, worldY: number) => {
    const pos = snapGrid ? snapPoint(worldX, worldY, level.tile_size) : { x: worldX, y: worldY };
    update((prev) => ({
      ...prev,
      objects: prev.objects.map((o, i) => (i === index ? { ...o, x: pos.x, y: pos.y } : o)),
    }));
  };

  const loadFile = async (file: File) => {
    try {
      const parsed = parseLevelJson(await file.text());
      replace(parsed, true);
      setSelectedObject(-1);
      setMapRevision((v) => v + 1);
      setStatus(`Loaded ${file.name}`);
    } catch (error) {
      setStatus(error instanceof Error ? error.message : "Failed to load file");
    }
  };

  const resizeMap = (width: number, height: number) => {
    update((prev) => resizeTiles(prev, width, height));
    setMapRevision((v) => v + 1);
    setStatus(`Map resized to ${width}×${height}`);
  };

  const updateObject = (patch: Partial<LevelObject>) => {
    if (selectedObject < 0) return;
    update((prev) => ({
      ...prev,
      objects: prev.objects.map((o, i) => (i === selectedObject ? { ...o, ...patch } : o)),
    }));
  };

  return (
    <div className="app">
      <header className="topbar">
        <div className="brand">
          <span className="brand-mark">R</span>
          <div>
            <strong>Rivet Level Editor</strong>
            <span className="muted">rivet-level v1 · web · JSON export</span>
          </div>
        </div>
        <div className="topbar-actions">
          <button type="button" disabled={!canUndo} onClick={() => { undo(); setStatus("Undo"); }}>
            Undo
          </button>
          <button type="button" disabled={!canRedo} onClick={() => { redo(); setStatus("Redo"); }}>
            Redo
          </button>
          <button type="button" onClick={() => fileInputRef.current?.click()}>
            Open
          </button>
          <button
            type="button"
            onClick={() => {
              replace(createBlankLevel());
              setSelectedObject(-1);
              setMapRevision((v) => v + 1);
              setStatus("New blank level");
            }}
          >
            New
          </button>
          <button type="button" className={showJson ? "active" : ""} onClick={() => setShowJson((v) => !v)}>
            JSON
          </button>
          <button type="button" className="primary" onClick={exportLevel}>
            Export
          </button>
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
      </header>

      <div className={`workspace ${showJson ? "with-json" : ""}`}>
        <Sidebar
          level={level}
          tool={tool}
          paintTile={paintTile}
          dirty={dirty}
          snapGrid={snapGrid}
          status={status}
          selected={selected}
          selectedIndex={selectedObject}
          onToolChange={setTool}
          onPaintTileChange={setPaintTile}
          onSnapGridChange={setSnapGrid}
          onLevelPatch={(patch) => update((prev) => ({ ...prev, ...patch }))}
          onResizeMap={resizeMap}
          onSelectObject={setSelectedObject}
          onObjectPatch={updateObject}
          onDeleteObject={deleteSelected}
        />
        <LevelCanvas
          level={level}
          tool={tool}
          selectedObject={selectedObject}
          mapRevision={mapRevision}
          onPointer={onCanvasPointer}
          onObjectDrag={onObjectDrag}
        />
        {showJson && <JsonPreview level={level} />}
      </div>
    </div>
  );
}
