import { useEffect, useRef } from "react";
import { tileIndexToAtlasCell } from "../lib/tile-math";
import type { TilesetDef } from "../types/tileset";

interface Props {
  tileset: TilesetDef | null;
  image: HTMLImageElement | null;
  tileId: number;
  size?: number;
  className?: string;
}

export function TilePreview({
  tileset,
  image,
  tileId,
  size = 48,
  className = "tile-preview-canvas",
}: Props) {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas || !tileset || !image) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    canvas.width = size;
    canvas.height = size;
    ctx.imageSmoothingEnabled = false;
    ctx.clearRect(0, 0, size, size);

    const { col, row } = tileIndexToAtlasCell(tileId, tileset.columns);
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
  }, [image, size, tileId, tileset]);

  return <canvas ref={canvasRef} className={className} width={size} height={size} />;
}
