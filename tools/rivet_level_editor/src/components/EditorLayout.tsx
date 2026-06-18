import { useEffect, useRef, useState, type ReactNode } from "react";
import type {
  CollisionTool,
  EditorMode,
  LevelData,
  LevelObject,
  ObjectTool,
  TileTool,
} from "../types/level";
import type { TilesetDef } from "../types/tileset";

interface MenuProps {
  dirty: boolean;
  canUndo: boolean;
  canRedo: boolean;
  showJson: boolean;
  onUndo: () => void;
  onRedo: () => void;
  onOpen: () => void;
  onNew: () => void;
  onExport: () => void;
  onToggleJson: () => void;
}

export function MenuBar({
  dirty,
  canUndo,
  canRedo,
  showJson,
  onUndo,
  onRedo,
  onOpen,
  onNew,
  onExport,
  onToggleJson,
}: MenuProps) {
  return (
    <header className="menu-bar">
      <div className="menu-brand">
        <span className="brand-mark">R</span>
        <span>Rivet Editor</span>
        {dirty && <span className="badge-dot" title="Unsaved changes" />}
      </div>
      <nav className="menu-nav">
        <button type="button" onClick={onNew}>
          New
        </button>
        <button type="button" onClick={onOpen}>
          Open
        </button>
        <button type="button" className="accent" onClick={onExport}>
          Export
        </button>
        <span className="menu-sep" />
        <button type="button" disabled={!canUndo} onClick={onUndo}>
          Undo
        </button>
        <button type="button" disabled={!canRedo} onClick={onRedo}>
          Redo
        </button>
        <span className="menu-sep" />
        <button type="button" className={showJson ? "active" : ""} onClick={onToggleJson}>
          JSON
        </button>
      </nav>
    </header>
  );
}

interface ToolRailProps {
  mode: EditorMode;
  tileTool: TileTool;
  collisionTool: CollisionTool;
  objectTool: ObjectTool;
  onModeChange: (mode: EditorMode) => void;
  onTileToolChange: (tool: TileTool) => void;
  onCollisionToolChange: (tool: CollisionTool) => void;
  onObjectToolChange: (tool: ObjectTool) => void;
}

export function ToolRail({
  mode,
  tileTool,
  collisionTool,
  objectTool,
  onModeChange,
  onTileToolChange,
  onCollisionToolChange,
  onObjectToolChange,
}: ToolRailProps) {
  const subTool =
    mode === "tiles" ? tileTool : mode === "collision" ? collisionTool : objectTool;

  const setSubTool = (id: string) => {
    if (mode === "tiles") onTileToolChange(id as TileTool);
    else if (mode === "collision") onCollisionToolChange(id as CollisionTool);
    else onObjectToolChange(id as ObjectTool);
  };

  const subTools =
    mode === "tiles"
      ? [
          { id: "paint", label: "Brush", icon: "▣" },
          { id: "erase", label: "Erase", icon: "◇" },
          { id: "select", label: "Select", icon: "↖" },
        ]
      : mode === "collision"
        ? [
            { id: "paint", label: "Solid", icon: "■" },
            { id: "erase", label: "Clear", icon: "□" },
          ]
        : [
            { id: "select", label: "Select", icon: "↖" },
            { id: "place-player", label: "Player", icon: "P" },
            { id: "place-block", label: "Block", icon: "B" },
          ];

  return (
    <aside className="tool-rail">
      <div className="tool-rail-modes">
        <button
          type="button"
          className={mode === "tiles" ? "active" : ""}
          title="Tiles (T)"
          onClick={() => onModeChange("tiles")}
        >
          <span className="tool-icon">▦</span>
          <span>Tiles</span>
        </button>
        <button
          type="button"
          className={mode === "collision" ? "active" : ""}
          title="Collision (C)"
          onClick={() => onModeChange("collision")}
        >
          <span className="tool-icon">⬚</span>
          <span>Collide</span>
        </button>
        <button
          type="button"
          className={mode === "objects" ? "active" : ""}
          title="Objects (O)"
          onClick={() => onModeChange("objects")}
        >
          <span className="tool-icon">◎</span>
          <span>Objects</span>
        </button>
      </div>
      <div className="tool-rail-sep" />
      <div className="tool-rail-sub">
        {subTools.map((t) => (
          <button
            key={t.id}
            type="button"
            className={subTool === t.id ? "active" : ""}
            title={t.label}
            onClick={() => setSubTool(t.id)}
          >
            <span className="tool-icon">{t.icon}</span>
          </button>
        ))}
      </div>
    </aside>
  );
}

interface TilesetDockProps {
  tilesets: Map<string, TilesetDef>;
  tilesetImages: Map<string, HTMLImageElement>;
  activeTilesetId: string;
  selectedTileId: number;
  onTilesetChange: (id: string) => void;
  onOpenPicker: () => void;
}

function TilePreview({
  tileset,
  image,
  tileId,
}: {
  tileset: TilesetDef | null;
  image: HTMLImageElement | null;
  tileId: number;
}) {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas || !tileset || !image) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    const size = 48;
    canvas.width = size;
    canvas.height = size;
    ctx.imageSmoothingEnabled = false;
    ctx.clearRect(0, 0, size, size);

    const col = tileId % tileset.columns;
    const row = Math.floor(tileId / tileset.columns);
    ctx.drawImage(
      image,
      col * tileset.tile_width,
      row * tileset.tile_height,
      tileset.tile_width,
      tileset.tile_height,
      0,
      0,
      size,
      size,
    );
  }, [image, tileId, tileset]);

  return <canvas ref={canvasRef} className="tile-preview-canvas" width={48} height={48} />;
}

export function TilesetDock({
  tilesets,
  tilesetImages,
  activeTilesetId,
  selectedTileId,
  onTilesetChange,
  onOpenPicker,
}: TilesetDockProps) {
  const tileset = tilesets.get(activeTilesetId) ?? null;
  const image = tilesetImages.get(activeTilesetId) ?? null;

  return (
    <footer className="tileset-dock">
      <label className="dock-label">
        Atlas
        <select value={activeTilesetId} onChange={(e) => onTilesetChange(e.target.value)}>
          {[...tilesets.values()].map((ts) => (
            <option key={ts.id} value={ts.id}>
              {ts.name}
            </option>
          ))}
        </select>
      </label>
      <button type="button" className="tile-preview-btn" onClick={onOpenPicker}>
        <TilePreview tileset={tileset} image={image} tileId={selectedTileId} />
        <span className="tile-preview-hint">Click to open atlas · tile #{selectedTileId}</span>
      </button>
    </footer>
  );
}

interface InspectorProps {
  level: LevelData;
  mode: EditorMode;
  selected: LevelObject | null;
  selectedPlacement: number;
  snapGrid: boolean;
  onLevelPatch: (patch: Partial<LevelData>) => void;
  onResizeMap: (w: number, h: number) => void;
  onObjectPatch: (patch: Partial<LevelObject>) => void;
  onDeleteObject: () => void;
  onDeletePlacement: () => void;
  onSnapGridChange: (v: boolean) => void;
}

export function Inspector({
  level,
  mode,
  selected,
  selectedPlacement,
  snapGrid,
  onLevelPatch,
  onResizeMap,
  onObjectPatch,
  onDeleteObject,
  onDeletePlacement,
  onSnapGridChange,
}: InspectorProps) {
  const [draftW, setDraftW] = useState(level.width);
  const [draftH, setDraftH] = useState(level.height);

  useEffect(() => {
    setDraftW(level.width);
    setDraftH(level.height);
  }, [level.width, level.height]);

  const placement = selectedPlacement >= 0 ? level.tiles[selectedPlacement] : null;

  return (
    <aside className="inspector">
      <section className="inspector-section">
        <h3>Map</h3>
        <label>
          Name
          <input value={level.name} onChange={(e) => onLevelPatch({ name: e.target.value })} />
        </label>
        <div className="field-row">
          <label>
            W
            <input
              type="number"
              min={4}
              max={512}
              value={draftW}
              onChange={(e) => setDraftW(Number(e.target.value))}
              onBlur={() => onResizeMap(draftW, draftH)}
            />
          </label>
          <label>
            H
            <input
              type="number"
              min={4}
              max={512}
              value={draftH}
              onChange={(e) => setDraftH(Number(e.target.value))}
              onBlur={() => onResizeMap(draftW, draftH)}
            />
          </label>
        </div>
        <p className="hint mono">
          {level.width}×{level.height} cells · {level.width * level.grid_size}px wide
        </p>
        <label>
          Background
          <input
            value={level.background}
            onChange={(e) => onLevelPatch({ background: e.target.value })}
            placeholder="path/to/sky.png"
          />
        </label>
        <label>
          Music
          <input
            value={level.music}
            onChange={(e) => onLevelPatch({ music: e.target.value })}
            placeholder="path/to/track.ogg"
          />
        </label>
        <label className="check-row">
          <input
            type="checkbox"
            checked={snapGrid}
            onChange={(e) => onSnapGridChange(e.target.checked)}
          />
          Snap objects to grid
        </label>
      </section>

      {mode === "tiles" && placement && (
        <section className="inspector-section">
          <h3>Selected tile</h3>
          <p className="mono">
            {placement.tileset}:{placement.id} @ {placement.x},{placement.y}
          </p>
          <button type="button" className="danger-btn" onClick={onDeletePlacement}>
            Delete tile
          </button>
        </section>
      )}

      {mode === "objects" && selected && (
        <section className="inspector-section">
          <h3>Object</h3>
          <label>
            id
            <input value={selected.id} onChange={(e) => onObjectPatch({ id: e.target.value })} />
          </label>
          <label>
            type
            <input
              value={selected.type}
              onChange={(e) => onObjectPatch({ type: e.target.value })}
            />
          </label>
          <div className="field-row">
            <label>
              x
              <input
                type="number"
                value={selected.x}
                onChange={(e) => onObjectPatch({ x: Number(e.target.value) })}
              />
            </label>
            <label>
              y
              <input
                type="number"
                value={selected.y}
                onChange={(e) => onObjectPatch({ y: Number(e.target.value) })}
              />
            </label>
          </div>
          <button type="button" className="danger-btn" onClick={onDeleteObject}>
            Delete object
          </button>
        </section>
      )}

      <section className="inspector-section">
        <h3>Stats</h3>
        <p className="hint">
          Tiles: {level.tiles.length} · Objects: {level.objects.length}
        </p>
      </section>
    </aside>
  );
}

export function StatusBar({ text, children }: { text: string; children?: ReactNode }) {
  return (
    <footer className="status-bar">
      <span>{text}</span>
      {children}
    </footer>
  );
}
