import { useEffect, useRef, useState, type ReactNode } from "react";
import { DEFAULT_FRAME_MS, type PlacedTile } from "../types/level";
import type {
  EditorMode,
  FluidPaint,
  GridTool,
  LevelData,
  LevelObject,
  ObjectTool,
  PaintFluidOption,
} from "../types/level";
import { isAnimatedPlacement, placementFrames as getPlacementFrames } from "../lib/tile-animation";
import { clampBrushSize } from "../lib/brush";
import type { TilesetDef } from "../types/tileset";
import { TilePreview } from "./TilePreview";
import { ContentAssetField } from "./ContentAssetField";
import type { PackAsset } from "../lib/resource-pack";

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
  onModeChange: (mode: EditorMode) => void;
}

export function ToolRail({ mode, onModeChange }: ToolRailProps) {
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
          className={mode === "fluids" ? "active" : ""}
          title="Fluids (F)"
          onClick={() => onModeChange("fluids")}
        >
          <span className="tool-icon">≈</span>
          <span>Fluids</span>
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
    </aside>
  );
}

const GRID_TOOLS: Array<{ id: GridTool; label: string }> = [
  { id: "select", label: "Select" },
  { id: "paint", label: "Brush" },
  { id: "line", label: "Line" },
  { id: "fill", label: "Fill" },
  { id: "erase", label: "Erase" },
];

const OBJECT_TOOLS: Array<{ id: ObjectTool; label: string }> = [
  { id: "select", label: "Select" },
  { id: "place-player", label: "Player" },
  { id: "place-block", label: "Block" },
];

interface OptionsBarProps {
  mode: EditorMode;
  gridTool: GridTool;
  objectTool: ObjectTool;
  fluidPaint: FluidPaint;
  brushSize: number;
  onGridToolChange: (tool: GridTool) => void;
  onObjectToolChange: (tool: ObjectTool) => void;
  onFluidPaintChange: (paint: FluidPaint) => void;
  onBrushSizeChange: (size: number) => void;
}

export function OptionsBar({
  mode,
  gridTool,
  objectTool,
  fluidPaint,
  brushSize,
  onGridToolChange,
  onObjectToolChange,
  onFluidPaintChange,
  onBrushSizeChange,
}: OptionsBarProps) {
  const size = clampBrushSize(brushSize);
  const showBrushSize =
    mode === "collision" || mode === "fluids" || (mode === "tiles" && gridTool === "erase");

  return (
    <div className="options-bar">
      {mode === "objects" ? (
        <div className="options-tools">
          {OBJECT_TOOLS.map((tool) => (
            <button
              key={tool.id}
              type="button"
              className={objectTool === tool.id ? "active" : ""}
              onClick={() => onObjectToolChange(tool.id)}
            >
              {tool.label}
            </button>
          ))}
        </div>
      ) : (
        <>
          <div className="options-tools">
            {GRID_TOOLS.map((tool) => (
              <button
                key={tool.id}
                type="button"
                className={gridTool === tool.id ? "active" : ""}
                onClick={() => onGridToolChange(tool.id)}
              >
                {tool.label}
              </button>
            ))}
          </div>
          {mode === "fluids" && gridTool !== "erase" && (
            <label className="options-field">
              <span>Type</span>
              <select
                value={fluidPaint}
                onChange={(e) => onFluidPaintChange(e.target.value as FluidPaint)}
              >
                <option value="water">Water</option>
                <option value="acid">Acid</option>
                <option value="lava">Lava</option>
              </select>
            </label>
          )}
        </>
      )}

      {showBrushSize && (
        <>
          <span className="options-sep" />
          <label className="options-field">
            <span>Size</span>
            <input
              type="range"
              min={1}
              max={16}
              value={size}
              onChange={(e) => onBrushSizeChange(clampBrushSize(Number(e.target.value)))}
            />
            <input
              type="number"
              min={1}
              max={16}
              value={size}
              onChange={(e) => onBrushSizeChange(clampBrushSize(Number(e.target.value)))}
            />
          </label>
          <span className="options-hint">{size}×{size}</span>
        </>
      )}
    </div>
  );
}

interface TilesetDockProps {
  tilesets: Map<string, TilesetDef>;
  tilesetImages: Map<string, HTMLImageElement>;
  activeTilesetId: string;
  selectedTileId: number;
  paintCollision: boolean;
  paintFluid: PaintFluidOption;
  onTilesetChange: (id: string) => void;
  onOpenPicker: () => void;
  onPaintCollisionChange: (value: boolean) => void;
  onPaintFluidChange: (value: PaintFluidOption) => void;
}

export function TilesetDock({
  tilesets,
  tilesetImages,
  activeTilesetId,
  selectedTileId,
  paintCollision,
  paintFluid,
  onTilesetChange,
  onOpenPicker,
  onPaintCollisionChange,
  onPaintFluidChange,
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
      <div className="dock-layers">
        <label className="check-row dock-check">
          <input
            type="checkbox"
            checked={paintCollision}
            onChange={(e) => onPaintCollisionChange(e.target.checked)}
          />
          Collision
        </label>
        <label className="dock-label dock-fluid">
          Fluid
          <select
            value={paintFluid}
            onChange={(e) => onPaintFluidChange(e.target.value as PaintFluidOption)}
          >
            <option value="none">None</option>
            <option value="water">Water</option>
            <option value="acid">Acid</option>
            <option value="lava">Lava</option>
          </select>
        </label>
      </div>
    </footer>
  );
}

interface InspectorProps {
  level: LevelData;
  mode: EditorMode;
  selected: LevelObject | null;
  selectedPlacement: number;
  selectedPlacementCount: number;
  tilesets: Map<string, TilesetDef>;
  tilesetImages: Map<string, HTMLImageElement>;
  backgroundAssets: PackAsset[];
  soundAssets: PackAsset[];
  snapGrid: boolean;
  onLevelPatch: (patch: Partial<LevelData>) => void;
  onResizeMap: (w: number, h: number) => void;
  onObjectPatch: (patch: Partial<LevelObject>) => void;
  onUpdatePlacement: (index: number, patch: Partial<PlacedTile>) => void;
  onDeleteObject: () => void;
  onDeletePlacement: () => void;
  onAddFrame: () => void;
  onSnapGridChange: (v: boolean) => void;
  onGridSizeChange: (gridSize: number) => void;
}

export function Inspector({
  level,
  mode,
  selected,
  selectedPlacement,
  selectedPlacementCount,
  tilesets,
  tilesetImages,
  backgroundAssets,
  soundAssets,
  snapGrid,
  onLevelPatch,
  onResizeMap,
  onObjectPatch,
  onUpdatePlacement,
  onDeleteObject,
  onDeletePlacement,
  onAddFrame,
  onSnapGridChange,
  onGridSizeChange,
}: InspectorProps) {
  const [draftW, setDraftW] = useState(level.width);
  const [draftH, setDraftH] = useState(level.height);
  const [draftGrid, setDraftGrid] = useState(level.grid_size);

  useEffect(() => {
    setDraftW(level.width);
    setDraftH(level.height);
  }, [level.width, level.height]);

  useEffect(() => {
    setDraftGrid(level.grid_size);
  }, [level.grid_size]);

  const placement = selectedPlacement >= 0 ? level.tiles[selectedPlacement] : null;
  const frames = placement ? getPlacementFrames(placement) : [];

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
              min={64}
              max={8192}
              value={draftW}
              onChange={(e) => setDraftW(Number(e.target.value))}
              onBlur={() => onResizeMap(draftW, draftH)}
            />
          </label>
          <label>
            H
            <input
              type="number"
              min={64}
              max={8192}
              value={draftH}
              onChange={(e) => setDraftH(Number(e.target.value))}
              onBlur={() => onResizeMap(draftW, draftH)}
            />
          </label>
        </div>
        <p className="hint mono">
          {level.width}×{level.height} px · snap {level.grid_size}px
        </p>
        <p className="hint">
          Resource pack: <span className="mono">{level.resource_pack || "dev"}</span>
          {" · "}
          {tilesets.size} tilesets, {backgroundAssets.length} backgrounds, {soundAssets.length} tracks
        </p>
        <ContentAssetField
          label="Background"
          value={level.background}
          assets={backgroundAssets}
          variant="image"
          emptyHint="Add images to src/resourcepacks/dev/textures/backgrounds/"
          onChange={(background) => onLevelPatch({ background })}
        />
        <ContentAssetField
          label="Music"
          value={level.music}
          assets={soundAssets}
          variant="audio"
          emptyHint="Add tracks to src/resourcepacks/dev/audio/music/"
          onChange={(music) => onLevelPatch({ music })}
        />
        <label className="check-row">
          <input
            type="checkbox"
            checked={snapGrid}
            onChange={(e) => onSnapGridChange(e.target.checked)}
          />
          Snap to grid
        </label>
        <label>
          Grid step (px)
          <input
            type="number"
            min={1}
            max={128}
            step={1}
            value={draftGrid}
            onChange={(e) => setDraftGrid(Number(e.target.value))}
            onBlur={() => onGridSizeChange(draftGrid)}
            onKeyDown={(e) => {
              if (e.key === "Enter") onGridSizeChange(draftGrid);
            }}
          />
        </label>
      </section>

      {mode === "tiles" && (placement || selectedPlacementCount > 0) && (
        <section className="inspector-section">
          <h3>Selected tile{selectedPlacementCount > 1 ? "s" : ""}</h3>
          {selectedPlacementCount > 1 ? (
            <p className="hint">{selectedPlacementCount} tiles selected · drag to move</p>
          ) : placement ? (
            <p className="mono">
              {placement.tileset}:{placement.id} @ {placement.x},{placement.y}
            </p>
          ) : null}

          {placement && (
            <>
              <div className="frame-list-header">
                <h4>Animation frames</h4>
                <button type="button" className="accent-btn" onClick={onAddFrame}>
                  + Add frame
                </button>
              </div>

              {isAnimatedPlacement(placement) && (
                <label>
                  Frame duration (ms)
                  <input
                    type="number"
                    min={40}
                    max={2000}
                    step={10}
                    value={placement.frame_ms ?? DEFAULT_FRAME_MS}
                    onChange={(e) =>
                      onUpdatePlacement(selectedPlacement, {
                        frame_ms: Number(e.target.value),
                      })
                    }
                  />
                </label>
              )}

              <ul className="frame-list">
                {frames.map((frame, index) => {
                  const frameTileset = tilesets.get(frame.tileset) ?? null;
                  const frameImage = tilesetImages.get(frame.tileset) ?? null;
                  return (
                    <li key={`${index}-${frame.tileset}-${frame.id}`} className="frame-list-item">
                      <TilePreview
                        tileset={frameTileset}
                        image={frameImage}
                        tileId={frame.id}
                        size={36}
                        className="frame-preview-canvas"
                      />
                      <div className="frame-list-meta">
                        <span className="mono">
                          #{index} {frame.tileset}:{frame.id}
                        </span>
                        {index === 0 && <span className="hint">primary</span>}
                      </div>
                      {index > 0 && (
                        <button
                          type="button"
                          className="frame-remove-btn"
                          title="Remove frame"
                          onClick={() => {
                            const nextFrames = frames.filter((_, i) => i !== index);
                            onUpdatePlacement(selectedPlacement, {
                              frames: nextFrames.length > 1 ? nextFrames : undefined,
                              frame_ms:
                                nextFrames.length > 1
                                  ? (placement.frame_ms ?? DEFAULT_FRAME_MS)
                                  : undefined,
                            });
                          }}
                        >
                          ×
                        </button>
                      )}
                    </li>
                  );
                })}
              </ul>
            </>
          )}

          <button type="button" className="danger-btn" onClick={onDeletePlacement}>
            Delete tile{selectedPlacementCount > 1 ? "s" : ""}
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
          <div className="field-row">
            <label>
              width
              <input
                type="number"
                min={1}
                value={selected.width}
                onChange={(e) => onObjectPatch({ width: Number(e.target.value) })}
              />
            </label>
            <label>
              height
              <input
                type="number"
                min={1}
                value={selected.height}
                onChange={(e) => onObjectPatch({ height: Number(e.target.value) })}
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
