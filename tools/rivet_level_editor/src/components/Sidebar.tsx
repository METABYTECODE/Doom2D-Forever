import { useEffect, useState } from "react";
import type { EditorTool, LevelData, LevelObject } from "../types/level";
import { clampMapSize } from "../lib/level-tiles";

const TILE_SWATCHES: { id: number; label: string; color: string }[] = [
  { id: 0, label: "Empty", color: "#1c202a" },
  { id: 1, label: "Wall", color: "#464e60" },
  { id: 2, label: "Dirt", color: "#5a4038" },
  { id: 3, label: "Grass", color: "#2f5a48" },
];

interface Props {
  level: LevelData;
  tool: EditorTool;
  paintTile: number;
  dirty: boolean;
  snapGrid: boolean;
  status: string;
  selected: LevelObject | null;
  selectedIndex: number;
  onToolChange: (tool: EditorTool) => void;
  onPaintTileChange: (tile: number) => void;
  onSnapGridChange: (value: boolean) => void;
  onLevelPatch: (patch: Partial<LevelData>) => void;
  onResizeMap: (width: number, height: number) => void;
  onSelectObject: (index: number) => void;
  onObjectPatch: (patch: Partial<LevelObject>) => void;
  onDeleteObject: () => void;
}

const TOOLS: { id: EditorTool; label: string; hint: string; key: string }[] = [
  { id: "paint", label: "Paint", hint: "LMB", key: "1" },
  { id: "erase", label: "Erase", hint: "LMB", key: "2" },
  { id: "place-player", label: "Player", hint: "LMB place", key: "3" },
  { id: "place-block", label: "Block", hint: "LMB place", key: "4" },
  { id: "select", label: "Select", hint: "drag · RMB del", key: "5" },
];

export function Sidebar({
  level,
  tool,
  paintTile,
  dirty,
  snapGrid,
  status,
  selected,
  selectedIndex,
  onToolChange,
  onPaintTileChange,
  onSnapGridChange,
  onLevelPatch,
  onResizeMap,
  onSelectObject,
  onObjectPatch,
  onDeleteObject,
}: Props) {
  const [draftWidth, setDraftWidth] = useState(level.width);
  const [draftHeight, setDraftHeight] = useState(level.height);

  useEffect(() => {
    setDraftWidth(level.width);
    setDraftHeight(level.height);
  }, [level.width, level.height]);

  const commitMapSize = () => {
    onResizeMap(clampMapSize(draftWidth), clampMapSize(draftHeight));
  };

  return (
    <aside className="sidebar">
      <section>
        <h2>Level</h2>
        {dirty && <p className="badge">unsaved</p>}
        <label>
          Name
          <input value={level.name} onChange={(e) => onLevelPatch({ name: e.target.value })} />
        </label>
        <div className="row">
          <label>
            W
            <input
              type="number"
              min={4}
              max={256}
              value={draftWidth}
              onChange={(e) => setDraftWidth(Number(e.target.value))}
              onBlur={commitMapSize}
              onKeyDown={(e) => e.key === "Enter" && commitMapSize()}
            />
          </label>
          <label>
            H
            <input
              type="number"
              min={4}
              max={256}
              value={draftHeight}
              onChange={(e) => setDraftHeight(Number(e.target.value))}
              onBlur={commitMapSize}
              onKeyDown={(e) => e.key === "Enter" && commitMapSize()}
            />
          </label>
        </div>
        <button type="button" onClick={commitMapSize}>
          Apply map size
        </button>
        <label>
          Tile size
          <input
            type="number"
            min={8}
            max={128}
            value={level.tile_size}
            onChange={(e) => onLevelPatch({ tile_size: Number(e.target.value) })}
          />
        </label>
        <label className="checkbox">
          <input
            type="checkbox"
            checked={snapGrid}
            onChange={(e) => onSnapGridChange(e.target.checked)}
          />
          Snap objects to grid
        </label>
      </section>

      <section>
        <h2>Tools</h2>
        <div className="tool-grid">
          {TOOLS.map((t) => (
            <button
              key={t.id}
              type="button"
              className={tool === t.id ? "active" : ""}
              onClick={() => {
                onToolChange(t.id);
                if (t.id === "erase") onPaintTileChange(0);
              }}
            >
              <strong>
                {t.label} <span className="keycap">{t.key}</span>
              </strong>
              <span>{t.hint}</span>
            </button>
          ))}
        </div>
        {tool === "paint" && (
          <div className="tile-palette">
            {TILE_SWATCHES.filter((tile) => tile.id !== 0).map((tile) => (
              <button
                key={tile.id}
                type="button"
                className={paintTile === tile.id ? "active" : ""}
                onClick={() => onPaintTileChange(tile.id)}
                title={tile.label}
              >
                <span className="swatch" style={{ background: tile.color }} />
                <span>{tile.id}</span>
              </button>
            ))}
          </div>
        )}
      </section>

      <section>
        <h2>Objects ({level.objects.length})</h2>
        <ul className="object-list">
          {level.objects.map((object, index) => (
            <li key={`${object.id}-${index}`}>
              <button
                type="button"
                className={selectedIndex === index ? "active" : ""}
                onClick={() => {
                  onSelectObject(index);
                  onToolChange("select");
                }}
              >
                <span>{object.type}</span>
                <span className="mono">
                  {Math.round(object.x)}, {Math.round(object.y)}
                </span>
              </button>
            </li>
          ))}
        </ul>
        {selected && (
          <div className="object-form">
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
            <div className="row">
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
            <div className="row">
              <label>
                w
                <input
                  type="number"
                  value={selected.width}
                  onChange={(e) => onObjectPatch({ width: Number(e.target.value) })}
                />
              </label>
              <label>
                h
                <input
                  type="number"
                  value={selected.height}
                  onChange={(e) => onObjectPatch({ height: Number(e.target.value) })}
                />
              </label>
            </div>
            <div className="row">
              <label>
                vx
                <input
                  type="number"
                  value={selected.vel_x}
                  onChange={(e) => onObjectPatch({ vel_x: Number(e.target.value) })}
                />
              </label>
              <label>
                vy
                <input
                  type="number"
                  value={selected.vel_y}
                  onChange={(e) => onObjectPatch({ vel_y: Number(e.target.value) })}
                />
              </label>
            </div>
            <button type="button" className="danger" onClick={onDeleteObject}>
              Delete object
            </button>
          </div>
        )}
      </section>

      <section className="shortcuts">
        <h2>Shortcuts</h2>
        <p className="mono">Ctrl+S export · Ctrl+Z undo · Del delete</p>
        <p className="mono">Space+LMB pan · Wheel zoom · MMB pan</p>
      </section>

      <footer className="status">{status}</footer>
    </aside>
  );
}
