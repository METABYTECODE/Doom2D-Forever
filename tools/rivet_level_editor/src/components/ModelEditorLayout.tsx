import { MODELS_CATALOG_PATH } from "../lib/atlas-render";
import type { ModelAnimation, ModelData } from "../types/model";
import type { PackAsset } from "../lib/resource-pack";
import { animationToTileset } from "../lib/model-atlas";
import { TilePreview } from "./TilePreview";

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

export function ModelMenuBar({
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
        <span className="brand-mark">M</span>
        <span>Model Editor</span>
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

interface AnimationRailProps {
  model: ModelData;
  builtinAnimations: readonly string[];
  selectedAnim: string;
  onSelectAnim: (id: string) => void;
  onAddAnim: () => void;
  onRemoveAnim: (id: string) => void;
}

export function AnimationRail({
  model,
  builtinAnimations,
  selectedAnim,
  onSelectAnim,
  onAddAnim,
  onRemoveAnim,
}: AnimationRailProps) {
  const custom = Object.keys(model.animations)
    .filter((id) => !builtinAnimations.includes(id))
    .sort();

  return (
    <aside className="model-anim-rail">
      <div className="rail-header">
        <strong>Animations</strong>
        <button type="button" className="icon-btn" title="Add animation" onClick={onAddAnim}>
          +
        </button>
      </div>
      <ul className="anim-list">
        {builtinAnimations.map((id) => {
          const configured = model.animations[id] != null;
          return (
            <li key={id}>
              <button
                type="button"
                className={`anim-item ${selectedAnim === id ? "active" : ""} ${configured ? "configured" : ""}`}
                onClick={() => onSelectAnim(id)}
              >
                <span className="anim-id">{id}</span>
                {configured && <span className="anim-badge">●</span>}
              </button>
            </li>
          );
        })}
        {custom.length > 0 && <li className="anim-sep">Custom</li>}
        {custom.map((id) => (
          <li key={id} className="anim-custom-row">
            <button
              type="button"
              className={`anim-item ${selectedAnim === id ? "active" : ""} configured`}
              onClick={() => onSelectAnim(id)}
            >
              <span className="anim-id">{id}</span>
            </button>
            <button
              type="button"
              className="icon-btn danger"
              title="Remove animation"
              onClick={() => onRemoveAnim(id)}
            >
              ×
            </button>
          </li>
        ))}
      </ul>
    </aside>
  );
}

interface InspectorProps {
  model: ModelData;
  animation: ModelAnimation | null;
  animId: string;
  atlases: PackAsset[];
  atlasImages: Map<string, HTMLImageElement>;
  playing: boolean;
  frameIndex: number;
  onModelPatch: (patch: Partial<Pick<ModelData, "id" | "name" | "resource_pack">>) => void;
  onAnimPatch: (patch: Partial<ModelAnimation>) => void;
  onAtlasChange: (atlasId: string) => void;
  onAddFrame: () => void;
  onRemoveFrame: (index: number) => void;
  onFrameIdChange: (index: number, id: number) => void;
  onTogglePlay: () => void;
  onFrameIndexChange: (index: number) => void;
  onEnsureAnimation: () => void;
}

export function ModelInspector({
  model,
  animation,
  animId,
  atlases,
  atlasImages,
  playing,
  frameIndex,
  onModelPatch,
  onAnimPatch,
  onAtlasChange,
  onAddFrame,
  onRemoveFrame,
  onFrameIdChange,
  onTogglePlay,
  onFrameIndexChange,
  onEnsureAnimation,
}: InspectorProps) {
  const atlasAsset = animation ? atlases.find((a) => a.id === animation.atlas) : undefined;
  const atlasImage = animation ? (atlasImages.get(animation.atlas) ?? null) : null;
  const tileset =
    animation && atlasAsset ? animationToTileset(animation, atlasAsset.url) : null;

  return (
    <aside className="inspector model-inspector">
      <section className="inspector-section">
        <h3>Model</h3>
        <label>
          id
          <input value={model.id} onChange={(e) => onModelPatch({ id: e.target.value })} />
        </label>
        <label>
          name
          <input value={model.name} onChange={(e) => onModelPatch({ name: e.target.value })} />
        </label>
        <label>
          resource_pack
          <input
            value={model.resource_pack}
            onChange={(e) => onModelPatch({ resource_pack: e.target.value })}
          />
        </label>
        <p className="hint">
          Export → resourcepacks/{model.resource_pack}/{MODELS_CATALOG_PATH}/{model.id}.model.json
        </p>
      </section>

      <section className="inspector-section">
        <div className="section-head">
          <h3>{animId}</h3>
          {!animation && (
            <button type="button" className="accent" onClick={onEnsureAnimation}>
              Configure
            </button>
          )}
        </div>

        {!animation ? (
          <p className="hint">Pick atlas PNG from dock, set cell size, choose frames.</p>
        ) : (
          <>
            <label>
              atlas
              <select value={animation.atlas} onChange={(e) => onAtlasChange(e.target.value)}>
                <option value="">— pick atlas —</option>
                {atlases.map((a) => (
                  <option key={a.id} value={a.id}>
                    {a.label}
                  </option>
                ))}
              </select>
            </label>
            <div className="field-row">
              <label>
                cell w
                <input
                  type="number"
                  min={1}
                  value={animation.frame_width}
                  onChange={(e) => onAnimPatch({ frame_width: Number(e.target.value) })}
                />
              </label>
              <label>
                cell h
                <input
                  type="number"
                  min={1}
                  value={animation.frame_height}
                  onChange={(e) => onAnimPatch({ frame_height: Number(e.target.value) })}
                />
              </label>
            </div>
            <label>
              columns
              <input
                type="number"
                min={1}
                value={animation.columns}
                onChange={(e) => onAnimPatch({ columns: Number(e.target.value) })}
              />
            </label>
            <p className="hint">
              Hitbox = frame cell ({animation.frame_width}×{animation.frame_height}px) · use zoom to enlarge view
            </p>
            <div className="field-row">
              <label>
                frame_ms
                <input
                  type="number"
                  min={0}
                  value={animation.frame_ms}
                  onChange={(e) => onAnimPatch({ frame_ms: Number(e.target.value) })}
                />
              </label>
              <label className="checkbox-label">
                <input
                  type="checkbox"
                  checked={animation.loop}
                  onChange={(e) => onAnimPatch({ loop: e.target.checked })}
                />
                loop
              </label>
            </div>

            <div className="frame-toolbar">
              <strong>Frames ({animation.frames.length})</strong>
              <button type="button" onClick={onAddFrame} disabled={!animation.atlas}>
                + Add
              </button>
            </div>
            <ul className="frame-list">
              {animation.frames.map((frame, index) => (
                <li key={`${index}-${frame.id}`} className="frame-row">
                  <TilePreview tileset={tileset} image={atlasImage} tileId={frame.id} size={40} />
                  <input
                    className="frame-id-input"
                    type="number"
                    min={0}
                    value={frame.id}
                    onChange={(e) => onFrameIdChange(index, Number(e.target.value))}
                  />
                  <button
                    type="button"
                    className="icon-btn danger"
                    onClick={() => onRemoveFrame(index)}
                  >
                    ×
                  </button>
                </li>
              ))}
            </ul>

            <div className="preview-controls">
              <button type="button" onClick={onTogglePlay} disabled={animation.frames.length === 0}>
                {playing ? "Pause" : "Play"}
              </button>
              <input
                type="range"
                min={0}
                max={Math.max(0, animation.frames.length - 1)}
                value={frameIndex}
                disabled={animation.frames.length === 0}
                onChange={(e) => onFrameIndexChange(Number(e.target.value))}
              />
              <span className="mono">
                {frameIndex + 1}/{animation.frames.length || 1}
              </span>
            </div>
          </>
        )}
      </section>
    </aside>
  );
}

export function ModelStatusBar({ text }: { text: string }) {
  return <footer className="status-bar">{text}</footer>;
}

interface SpriteDockProps {
  atlases: PackAsset[];
  atlasImages: Map<string, HTMLImageElement>;
  animation: ModelAnimation | null;
  activeAtlasId: string;
  previewFrameId: number;
  animLabel: string;
  onAtlasChange: (id: string) => void;
  onOpenAtlas: () => void;
}

export function SpriteDock({
  atlases,
  atlasImages,
  animation,
  activeAtlasId,
  previewFrameId,
  animLabel,
  onAtlasChange,
  onOpenAtlas,
}: SpriteDockProps) {
  const atlasAsset = atlases.find((a) => a.id === activeAtlasId);
  const image = atlasImages.get(activeAtlasId) ?? null;
  const tileset = animation && atlasAsset ? animationToTileset(animation, atlasAsset.url) : null;

  return (
    <footer className="tileset-dock sprite-dock">
      <label className="dock-label">
        Atlas PNG · {animLabel}
        <select
          value={activeAtlasId}
          onChange={(e) => onAtlasChange(e.target.value)}
          disabled={atlases.length === 0}
        >
          {atlases.length === 0 ? (
            <option value="">No PNG in textures/sprites/</option>
          ) : (
            atlases.map((a) => (
              <option key={a.id} value={a.id}>
                {a.label}
              </option>
            ))
          )}
        </select>
      </label>
      <button
        type="button"
        className="tile-preview-btn"
        onClick={onOpenAtlas}
        disabled={!atlasAsset || atlases.length === 0}
      >
        <TilePreview tileset={tileset} image={image} tileId={previewFrameId} />
        <span className="tile-preview-hint">
          {atlasAsset
            ? `Click to open atlas · frame #${previewFrameId}`
            : "Drop PNG into textures/sprites/"}
        </span>
      </button>
      {animation && (
        <span className="dock-meta muted">
          {animation.frame_width}×{animation.frame_height} · {animation.columns} cols
        </span>
      )}
    </footer>
  );
}
