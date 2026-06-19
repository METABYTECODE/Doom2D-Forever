import { useEffect, useRef, useState } from "react";
import { drawAtlasFrame } from "../lib/atlas-render";
import type { ModelAnimation } from "../types/model";

interface Props {
  image: HTMLImageElement | null;
  animation: ModelAnimation | null;
  playing: boolean;
  frameIndex: number;
  onFrameIndexChange?: (index: number) => void;
}

export function ModelPreviewCanvas({
  image,
  animation,
  playing,
  frameIndex,
  onFrameIndexChange,
}: Props) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const wrapRef = useRef<HTMLDivElement>(null);
  const [zoom, setZoom] = useState(2);
  const playRef = useRef({ playing, frameIndex });

  playRef.current = { playing, frameIndex };

  useEffect(() => {
    const wrap = wrapRef.current;
    if (!wrap) return;
    const onWheel = (e: WheelEvent) => {
      e.preventDefault();
      const factor = e.deltaY < 0 ? 1.12 : 0.88;
      setZoom((z) => Math.min(12, Math.max(0.5, z * factor)));
    };
    wrap.addEventListener("wheel", onWheel, { passive: false });
    return () => wrap.removeEventListener("wheel", onWheel);
  }, []);

  useEffect(() => {
    if (!playing || !animation || animation.frames.length <= 1) return;
    let raf = 0;
    let last = performance.now();

    const tick = (now: number) => {
      const state = playRef.current;
      if (!state.playing) return;
      const frameMs = Math.max(1, animation.frame_ms);
      if (now - last >= frameMs) {
        last = now;
        const next = (state.frameIndex + 1) % animation.frames.length;
        if (!animation.loop && next === 0) {
          onFrameIndexChange?.(animation.frames.length - 1);
          return;
        }
        onFrameIndexChange?.(next);
      }
      raf = requestAnimationFrame(tick);
    };

    raf = requestAnimationFrame(tick);
    return () => cancelAnimationFrame(raf);
  }, [animation, playing, onFrameIndexChange]);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    const frameW = animation?.frame_width ?? 64;
    const frameH = animation?.frame_height ?? 64;
    const pad = 32;
    const drawW = Math.round(frameW * zoom);
    const drawH = Math.round(frameH * zoom);

    canvas.width = drawW + pad * 2;
    canvas.height = drawH + pad * 2;

    ctx.imageSmoothingEnabled = false;
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.fillStyle = "#1a1d24";
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    const gridStep = Math.max(4, Math.round(8 * zoom));
    ctx.strokeStyle = "rgba(255,255,255,0.06)";
    ctx.lineWidth = 1;
    for (let x = 0; x <= canvas.width; x += gridStep) {
      ctx.beginPath();
      ctx.moveTo(x, 0);
      ctx.lineTo(x, canvas.height);
      ctx.stroke();
    }
    for (let y = 0; y <= canvas.height; y += gridStep) {
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(canvas.width, y);
      ctx.stroke();
    }

    const originX = pad;
    const originY = pad;

    if (animation && image && animation.frames.length > 0) {
      const frameId = animation.frames[Math.min(frameIndex, animation.frames.length - 1)]?.id ?? 0;
      drawAtlasFrame(ctx, image, animation, frameId, originX, originY, zoom);

      ctx.strokeStyle = "#6ea8ff";
      ctx.lineWidth = 2;
      ctx.strokeRect(originX, originY, drawW, drawH);
      ctx.fillStyle = "rgba(110,168,255,0.1)";
      ctx.fillRect(originX, originY, drawW, drawH);
    } else {
      ctx.fillStyle = "rgba(255,255,255,0.06)";
      ctx.fillRect(originX, originY, drawW, drawH);
      ctx.strokeStyle = "rgba(255,255,255,0.2)";
      ctx.strokeRect(originX, originY, drawW, drawH);
    }
  }, [animation, frameIndex, image, zoom]);

  return (
    <div className="model-preview-wrap" ref={wrapRef}>
      <div className="model-preview-toolbar">
        <button type="button" onClick={() => setZoom((z) => Math.max(0.5, z - 0.5))}>
          −
        </button>
        <span className="mono">{zoom.toFixed(1)}×</span>
        <button type="button" onClick={() => setZoom((z) => Math.min(12, z + 0.5))}>
          +
        </button>
        <button type="button" onClick={() => setZoom(2)}>
          Reset
        </button>
      </div>
      <canvas ref={canvasRef} className="model-preview-canvas" />
      <p className="muted model-preview-hint">
        Frame {animation?.frame_width ?? "?"}×{animation?.frame_height ?? "?"}px at {zoom.toFixed(1)}×
        · scroll to zoom view
      </p>
    </div>
  );
}
