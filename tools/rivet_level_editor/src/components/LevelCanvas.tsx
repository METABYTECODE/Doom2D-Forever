import { useEffect, useRef, useState } from "react";
import { hitObject } from "../lib/geometry";
import type { EditorTool, LevelData } from "../types/level";

interface Props {
  level: LevelData;
  tool: EditorTool;
  selectedObject: number;
  mapRevision: number;
  onPointer: (worldX: number, worldY: number, button: number) => void;
  onObjectDrag: (index: number, worldX: number, worldY: number) => void;
}

const TILE_COLORS: Record<number, string> = {
  0: "#1c202a",
  1: "#464e60",
  2: "#5a4038",
  3: "#2f5a48",
};

const OBJECT_COLORS: Record<string, string> = {
  player: "#dc5a46",
  block: "#78aa5a",
};

type DrawState = {
  camera: { x: number; y: number; zoom: number };
  hover: { wx: number; wy: number; tx: number; ty: number } | null;
  level: LevelData;
  selectedObject: number;
  tool: EditorTool;
};

export function LevelCanvas({
  level,
  tool,
  selectedObject,
  mapRevision,
  onPointer,
  onObjectDrag,
}: Props) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const containerRef = useRef<HTMLDivElement>(null);
  const [camera, setCamera] = useState({ x: 0, y: 0, zoom: 1.5 });
  const [hoverTile, setHoverTile] = useState<{ tx: number; ty: number } | null>(null);

  const paintingRef = useRef(false);
  const spaceRef = useRef(false);
  const panRef = useRef({ active: false, sx: 0, sy: 0, cx: 0, cy: 0 });
  const dragRef = useRef<{ index: number; offsetX: number; offsetY: number } | null>(null);
  const cameraRef = useRef(camera);
  const drawStateRef = useRef<DrawState>({
    camera,
    hover: null,
    level,
    selectedObject,
    tool,
  });

  cameraRef.current = camera;
  drawStateRef.current = {
    camera,
    hover: hoverTile
      ? { wx: 0, wy: 0, tx: hoverTile.tx, ty: hoverTile.ty }
      : null,
    level,
    selectedObject,
    tool,
  };

  useEffect(() => {
    const worldW = level.width * level.tile_size;
    const worldH = level.height * level.tile_size;
    setCamera((c) => ({ ...c, x: worldW * 0.5, y: worldH * 0.5 }));
  }, [mapRevision, level.tile_size, level.width, level.height]);

  useEffect(() => {
    const onKeyDown = (e: KeyboardEvent) => {
      if (e.code === "Space") spaceRef.current = true;
    };
    const onKeyUp = (e: KeyboardEvent) => {
      if (e.code === "Space") spaceRef.current = false;
    };
    window.addEventListener("keydown", onKeyDown);
    window.addEventListener("keyup", onKeyUp);
    return () => {
      window.removeEventListener("keydown", onKeyDown);
      window.removeEventListener("keyup", onKeyUp);
    };
  }, []);

  useEffect(() => {
    const canvas = canvasRef.current;
    const container = containerRef.current;
    if (!canvas || !container) return;

    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    const resize = () => {
      const rect = container.getBoundingClientRect();
      const dpr = window.devicePixelRatio || 1;
      canvas.width = Math.floor(rect.width * dpr);
      canvas.height = Math.floor(rect.height * dpr);
      canvas.style.width = `${rect.width}px`;
      canvas.style.height = `${rect.height}px`;
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    };

    resize();
    const observer = new ResizeObserver(resize);
    observer.observe(container);

    let frame = 0;
    const draw = () => {
      const { camera: cam, hover, level: lvl, selectedObject: selected, tool: activeTool } =
        drawStateRef.current;

      const rect = container.getBoundingClientRect();
      const w = rect.width;
      const h = rect.height;

      ctx.fillStyle = "#12141c";
      ctx.fillRect(0, 0, w, h);

      const toScreen = (wx: number, wy: number) => ({
        x: w * 0.5 + (wx - cam.x) * cam.zoom,
        y: h * 0.5 + (wy - cam.y) * cam.zoom,
      });

      const ts = lvl.tile_size * cam.zoom;
      const rowCount = Math.min(lvl.height, lvl.tiles.length);

      for (let y = 0; y < rowCount; y++) {
        const row = lvl.tiles[y];
        if (!row) continue;
        const colCount = Math.min(lvl.width, row.length);
        for (let x = 0; x < colCount; x++) {
          const tile = row[x] ?? 0;
          const p = toScreen(x * lvl.tile_size, y * lvl.tile_size);
          ctx.fillStyle = TILE_COLORS[tile] ?? "#333";
          ctx.fillRect(p.x, p.y, ts, ts);
          ctx.strokeStyle = "rgba(255,255,255,0.05)";
          ctx.strokeRect(p.x, p.y, ts, ts);
        }
      }

      if (hover) {
        const hp = toScreen(hover.tx * lvl.tile_size, hover.ty * lvl.tile_size);
        ctx.strokeStyle = "rgba(110,168,255,0.75)";
        ctx.lineWidth = 2;
        ctx.strokeRect(hp.x, hp.y, ts, ts);
      }

      lvl.objects.forEach((object, index) => {
        const p = toScreen(object.x, object.y);
        const ow = object.width * cam.zoom;
        const oh = object.height * cam.zoom;
        ctx.fillStyle = OBJECT_COLORS[object.type] ?? "#c8c850";
        ctx.globalAlpha = index === selected ? 1 : 0.85;
        ctx.fillRect(p.x, p.y, ow, oh);
        ctx.globalAlpha = 1;
        ctx.strokeStyle = index === selected ? "#fff6a0" : "rgba(255,255,255,0.35)";
        ctx.lineWidth = index === selected ? 2 : 1;
        ctx.strokeRect(p.x, p.y, ow, oh);
        ctx.fillStyle = "#fff";
        ctx.font = `600 ${Math.max(10, 11 * cam.zoom)}px Outfit, sans-serif`;
        ctx.fillText(object.type, p.x + 4, p.y + 14 * cam.zoom);
      });

      ctx.fillStyle = "rgba(255,255,255,0.55)";
      ctx.font = "12px JetBrains Mono, monospace";
      const hoverText = hover ? ` · tile ${hover.tx},${hover.ty}` : "";
      ctx.fillText(`tool: ${activeTool} · zoom ${cam.zoom.toFixed(2)}${hoverText}`, 16, h - 16);

      frame = requestAnimationFrame(draw);
    };

    frame = requestAnimationFrame(draw);
    return () => {
      cancelAnimationFrame(frame);
      observer.disconnect();
    };
  }, []);

  const screenToWorld = (clientX: number, clientY: number) => {
    const rect = containerRef.current!.getBoundingClientRect();
    const cam = cameraRef.current;
    const sx = clientX - rect.left;
    const sy = clientY - rect.top;
    const w = rect.width;
    const h = rect.height;
    return {
      x: cam.x + (sx - w * 0.5) / cam.zoom,
      y: cam.y + (sy - h * 0.5) / cam.zoom,
    };
  };

  const updateHover = (clientX: number, clientY: number) => {
    const world = screenToWorld(clientX, clientY);
    const tx = Math.floor(world.x / level.tile_size);
    const ty = Math.floor(world.y / level.tile_size);
    setHoverTile((prev) => (prev?.tx === tx && prev?.ty === ty ? prev : { tx, ty }));
  };

  const strokeTool = tool === "paint" || tool === "erase";

  return (
    <div
      ref={containerRef}
      className="canvas-wrap"
      onContextMenu={(e) => e.preventDefault()}
      onWheel={(e) => {
        e.preventDefault();
        const factor = e.deltaY < 0 ? 1.08 : 0.92;
        setCamera((c) => ({ ...c, zoom: Math.min(4, Math.max(0.35, c.zoom * factor)) }));
      }}
      onPointerDown={(e) => {
        (e.currentTarget as HTMLDivElement).setPointerCapture(e.pointerId);
        const world = screenToWorld(e.clientX, e.clientY);

        if (e.button === 1 || (e.button === 0 && spaceRef.current)) {
          panRef.current = { active: true, sx: e.clientX, sy: e.clientY, cx: cameraRef.current.x, cy: cameraRef.current.y };
          return;
        }

        if (tool === "select" && e.button === 0) {
          const hit = hitObject(level.objects, world.x, world.y);
          if (hit >= 0) {
            const object = level.objects[hit];
            dragRef.current = {
              index: hit,
              offsetX: world.x - object.x,
              offsetY: world.y - object.y,
            };
            onPointer(world.x, world.y, e.button);
            return;
          }
        }

        if (strokeTool && e.button === 0) {
          paintingRef.current = true;
          onPointer(world.x, world.y, e.button);
          return;
        }

        onPointer(world.x, world.y, e.button);
      }}
      onPointerMove={(e) => {
        updateHover(e.clientX, e.clientY);

        if (panRef.current.active) {
          const cam = cameraRef.current;
          const dx = (e.clientX - panRef.current.sx) / cam.zoom;
          const dy = (e.clientY - panRef.current.sy) / cam.zoom;
          setCamera({ ...cam, x: panRef.current.cx - dx, y: panRef.current.cy - dy });
          return;
        }

        const world = screenToWorld(e.clientX, e.clientY);

        if (dragRef.current) {
          onObjectDrag(
            dragRef.current.index,
            world.x - dragRef.current.offsetX,
            world.y - dragRef.current.offsetY,
          );
          return;
        }

        if (!paintingRef.current) return;
        onPointer(world.x, world.y, e.button);
      }}
      onPointerUp={(e) => {
        paintingRef.current = false;
        panRef.current.active = false;
        dragRef.current = null;
        try {
          (e.currentTarget as HTMLDivElement).releasePointerCapture(e.pointerId);
        } catch {
          /* ignore */
        }
      }}
      onPointerLeave={() => {
        paintingRef.current = false;
        panRef.current.active = false;
        dragRef.current = null;
        setHoverTile(null);
      }}
    >
      <canvas ref={canvasRef} />
    </div>
  );
}
