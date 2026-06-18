import { useEffect, useRef, useState } from "react";
import { atlasCellToTileIndex, tileIndexToAtlasCell } from "../lib/tile-math";
import type { TilesetDef } from "../types/tileset";

interface Props {
  open: boolean;
  tileset: TilesetDef | null;
  image: HTMLImageElement | null;
  selectedTileId: number;
  onSelect?: (tileId: number) => void;
  onSelectFrame?: (tilesetId: string, tileId: number) => void;
  onClose: () => void;
  title?: string;
  tilesets?: Map<string, TilesetDef>;
  tilesetImages?: Map<string, HTMLImageElement>;
  activeTilesetId?: string;
  onTilesetChange?: (id: string) => void;
}

export function TilesetPickerModal({
  open,
  tileset,
  image,
  selectedTileId,
  onSelect,
  onSelectFrame,
  onClose,
  title = "Pick tile",
  tilesets,
  tilesetImages,
  activeTilesetId,
  onTilesetChange,
}: Props) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const wrapRef = useRef<HTMLDivElement>(null);
  const [zoom, setZoom] = useState(4);

  useEffect(() => {
    if (open) setZoom(4);
  }, [open, tileset?.id]);

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
      setZoom((z) => Math.min(16, Math.max(2, z * factor)));
    };
    wrap.addEventListener("wheel", onWheel, { passive: false });
    return () => wrap.removeEventListener("wheel", onWheel);
  }, [open]);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas || !tileset || !image || !open) return;

    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    const cellW = tileset.tile_width * zoom;
    const cellH = tileset.tile_height * zoom;
    const rows = tileset.rows ?? Math.ceil(image.height / tileset.tile_height);
    canvas.width = tileset.columns * cellW;
    canvas.height = rows * cellH;

    ctx.imageSmoothingEnabled = false;
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.drawImage(image, 0, 0, canvas.width, canvas.height);

    ctx.strokeStyle = "rgba(255,255,255,0.15)";
    ctx.lineWidth = 1;
    for (let row = 0; row < rows; row++) {
      for (let col = 0; col < tileset.columns; col++) {
        ctx.strokeRect(col * cellW, row * cellH, cellW, cellH);
      }
    }

    const { col, row } = tileIndexToAtlasCell(selectedTileId, tileset.columns);
    ctx.strokeStyle = "#6ea8ff";
    ctx.lineWidth = 3;
    ctx.strokeRect(col * cellW, row * cellH, cellW, cellH);
  }, [image, open, selectedTileId, tileset, zoom]);

  const pickTile = (event: React.PointerEvent<HTMLCanvasElement>) => {
    if (!tileset || !image) return;
    const canvas = canvasRef.current;
    if (!canvas) return;

    const rect = canvas.getBoundingClientRect();
    const scaleX = canvas.width / rect.width;
    const scaleY = canvas.height / rect.height;
    const px = (event.clientX - rect.left) * scaleX;
    const py = (event.clientY - rect.top) * scaleY;

    const cellW = tileset.tile_width * zoom;
    const cellH = tileset.tile_height * zoom;
    const col = Math.floor(px / cellW);
    const row = Math.floor(py / cellH);
    if (col < 0 || row < 0 || col >= tileset.columns) return;

    const tileId = atlasCellToTileIndex(col, row, tileset.columns);
    if (onSelectFrame) {
      onSelectFrame(activeTilesetId ?? tileset.id, tileId);
    } else {
      onSelect?.(tileId);
    }
    onClose();
  };

  if (!open || !tileset) return null;

  const { col, row } = tileIndexToAtlasCell(selectedTileId, tileset.columns);

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
            {tileset && (
              <span className="muted">
                {" "}
                · {tileset.name} · {tileset.tile_width}×{tileset.tile_height} · tile #{selectedTileId}{" "}
                (col {col}, row {row})
              </span>
            )}
          </div>
          <div className="tileset-modal-actions">
            {tilesets && onTilesetChange && (
              <select
                value={activeTilesetId ?? tileset?.id ?? ""}
                onChange={(e) => onTilesetChange(e.target.value)}
              >
                {[...tilesets.values()].map((ts) => (
                  <option key={ts.id} value={ts.id}>
                    {ts.name}
                  </option>
                ))}
              </select>
            )}
            <button type="button" onClick={() => setZoom((z) => Math.max(2, z - 1))}>
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
        <div ref={wrapRef} className="tileset-modal-body">
          {image ? (
            <canvas ref={canvasRef} className="tileset-modal-canvas" onPointerDown={pickTile} />
          ) : (
            <p className="muted">Texture not loaded for {tileset.id}</p>
          )}
        </div>
        <footer className="tileset-modal-footer muted">
          Click a cell to select · scroll to zoom · Esc to close
        </footer>
      </div>
    </div>
  );
}
