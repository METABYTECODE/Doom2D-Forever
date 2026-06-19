import type { ModelAnimation } from "../types/model";
import { tileIndexToAtlasCell } from "./tile-math";

/** Draw one atlas frame at native pixel size × zoom (no stretch). */
export function drawAtlasFrame(
  ctx: CanvasRenderingContext2D,
  image: HTMLImageElement,
  anim: Pick<ModelAnimation, "frame_width" | "frame_height" | "columns">,
  frameId: number,
  destX: number,
  destY: number,
  pixelZoom: number,
): { width: number; height: number } {
  const sw = Math.max(1, anim.frame_width);
  const sh = Math.max(1, anim.frame_height);
  const columns = Math.max(1, anim.columns);
  const { col, row } = tileIndexToAtlasCell(frameId, columns);
  const dw = Math.round(sw * pixelZoom);
  const dh = Math.round(sh * pixelZoom);
  ctx.drawImage(image, col * sw, row * sh, sw, sh, destX, destY, dw, dh);
  return { width: dw, height: dh };
}

/** Draw full atlas bitmap at native resolution × zoom. */
export function drawAtlasImage(
  ctx: CanvasRenderingContext2D,
  image: HTMLImageElement,
  pixelZoom: number,
): { width: number; height: number } {
  const w = Math.round(image.width * pixelZoom);
  const h = Math.round(image.height * pixelZoom);
  ctx.drawImage(image, 0, 0, w, h);
  return { width: w, height: h };
}

export const MODELS_CATALOG_PATH = "models";
