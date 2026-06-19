import { useEffect, useRef, useState } from "react";
import { drawAtlasFrame, drawAtlasImage } from "../lib/atlas-render";
import { atlasRows } from "../lib/model-atlas";
import { atlasCellToTileIndex, tileIndexToAtlasCell } from "../lib/tile-math";
import type { PackAsset } from "../lib/resource-pack";
import type { ModelAnimation } from "../types/model";

interface Props {
  open: boolean;
  title?: string;
  atlases: PackAsset[];
  atlasImages: Map<string, HTMLImageElement>;
  activeAtlasId: string;
  grid: Pick<ModelAnimation, "frame_width" | "frame_height" | "columns">;
  selectedFrameId: number;
  onAtlasChange: (id: string) => void;
  onGridChange: (patch: Partial<Pick<ModelAnimation, "frame_width" | "frame_height" | "columns">>) => void;
  onSelect: (atlasId: string, frameId: number) => void;
  onClose: () => void;
}

export function AtlasPickerModal({
  open,
  title = "Pick atlas frame",
  atlases,
  atlasImages,
  activeAtlasId,
  grid,
  selectedFrameId,
  onAtlasChange,
  onGridChange,
  onSelect,
  onClose,
}: Props) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const wrapRef = useRef<HTMLDivElement>(null);
  const [zoom, setZoom] = useState(4);

  const image = atlasImages.get(activeAtlasId) ?? null;
  const frameW = Math.max(1, grid.frame_width);
  const frameH = Math.max(1, grid.frame_height);
  const columns = Math.max(1, grid.columns);
  const rows = image ? atlasRows(image.height, frameH) : 1;

  useEffect(() => {
    if (open) setZoom(4);
  }, [open, activeAtlasId]);

  useEffect(() => {
    if (!open) return;
    const onKey = (e: KeyboardEvent) => {
      if (e.key === "Escape") onClose();
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [open, onClose]);

  useEffect(() => {
    const wrap = wrapRef.current;
    if (!wrap || !open) return;
    const onWheel = (e: WheelEvent) => {
      e.preventDefault();
      e.stopPropagation();
      const factor = e.deltaY < 0 ? 1.1 : 0.9;
      setZoom((z) => Math.min(16, Math.max(1, z * factor)));
    };
    wrap.addEventListener("wheel", onWheel, { passive: false });
    return () => wrap.removeEventListener("wheel", onWheel);
  }, [open]);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas || !image || !open) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    const atlasW = Math.round(image.width * zoom);
    const atlasH = Math.round(image.height * zoom);
    canvas.width = atlasW;
    canvas.height = atlasH;

    ctx.imageSmoothingEnabled = false;
    ctx.clearRect(0, 0, atlasW, atlasH);
    drawAtlasImage(ctx, image, zoom);

    const cellW = frameW * zoom;
    const cellH = frameH * zoom;
    ctx.strokeStyle = "rgba(255,255,255,0.2)";
    ctx.lineWidth = 1;
    for (let row = 0; row < rows; row++) {
      for (let col = 0; col < columns; col++) {
        const x = col * cellW;
        const y = row * cellH;
        if (x >= atlasW || y >= atlasH) break;
        ctx.strokeRect(x, y, Math.min(cellW, atlasW - x), Math.min(cellH, atlasH - y));
      }
    }

    const { col, row } = tileIndexToAtlasCell(selectedFrameId, columns);
    ctx.strokeStyle = "#6ea8ff";
    ctx.lineWidth = 3;
    ctx.strokeRect(col * cellW, row * cellH, cellW, cellH);
  }, [columns, frameH, frameW, image, open, rows, selectedFrameId, zoom]);

  const pickFrame = (event: React.PointerEvent<HTMLCanvasElement>) => {
    if (!image) return;
    const canvas = canvasRef.current;
    if (!canvas) return;

    const rect = canvas.getBoundingClientRect();
    const scaleX = canvas.width / rect.width;
    const scaleY = canvas.height / rect.height;
    const px = (event.clientX - rect.left) * scaleX;
    const py = (event.clientY - rect.top) * scaleY;

    const cellW = frameW * zoom;
    const cellH = frameH * zoom;
    const col = Math.floor(px / cellW);
    const row = Math.floor(py / cellH);
    if (col < 0 || row < 0 || col >= columns || row >= rows) return;

    onSelect(activeAtlasId, atlasCellToTileIndex(col, row, columns));
    onClose();
  };

  if (!open) return null;

  const { col, row } = tileIndexToAtlasCell(selectedFrameId, columns);

  return (
    <div className="modal-backdrop" onPointerDown={onClose}>
      <div
        className="tileset-modal"
        onPointerDown={(e) => e.stopPropagation()}
        role="dialog"
        aria-modal="true"
        aria-label={title}
      >
        <header className="tileset-modal-header">
          <div>
            <strong>{title}</strong>
            <span className="muted">
              {" "}
              · {image ? `${image.width}×${image.height}px` : ""} · frame #{selectedFrameId} (col{" "}
              {col}, row {row})
            </span>
          </div>
          <div className="tileset-modal-actions">
            <select value={activeAtlasId} onChange={(e) => onAtlasChange(e.target.value)}>
              {atlases.map((a) => (
                <option key={a.id} value={a.id}>
                  {a.label}
                </option>
              ))}
            </select>
            <button type="button" onClick={() => setZoom((z) => Math.max(1, z - 1))}>
              −
            </button>
            <span className="mono">{zoom}×</span>
            <button type="button" onClick={() => setZoom((z) => Math.min(16, z + 1))}>
              +
            </button>
            <button type="button" className="modal-close" onClick={onClose}>
              ✕
            </button>
          </div>
        </header>
        <div className="atlas-grid-bar">
          <label>
            cell w
            <input
              type="number"
              min={1}
              value={grid.frame_width}
              onChange={(e) => onGridChange({ frame_width: Number(e.target.value) })}
            />
          </label>
          <label>
            cell h
            <input
              type="number"
              min={1}
              value={grid.frame_height}
              onChange={(e) => onGridChange({ frame_height: Number(e.target.value) })}
            />
          </label>
          <label>
            columns
            <input
              type="number"
              min={1}
              value={grid.columns}
              onChange={(e) => onGridChange({ columns: Number(e.target.value) })}
            />
          </label>
          <span className="muted">Grid only — atlas scale stays fixed</span>
        </div>
        <div ref={wrapRef} className="tileset-modal-body">
          {image ? (
            <canvas ref={canvasRef} className="tileset-modal-canvas" onPointerDown={pickFrame} />
          ) : (
            <p className="muted">Texture not loaded for {activeAtlasId}</p>
          )}
        </div>
        <footer className="tileset-modal-footer muted">
          Click a cell · scroll to zoom atlas · cell size = grid step · Esc to close
        </footer>
      </div>
    </div>
  );
}
