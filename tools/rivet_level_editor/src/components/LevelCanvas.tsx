import { useEffect, useRef, useState } from "react";
import { animatedTileFrame } from "../lib/tile-animation";
import { worldCellSize } from "../lib/level-collision";
import { hitObject, tileAt } from "../lib/geometry";
import { tileCellSpan, tileIndexToAtlasCell } from "../lib/tile-math";
import type {
  CollisionTool,
  EditorMode,
  LevelData,
  ObjectTool,
  TileTool,
} from "../types/level";
import type { TilesetDef } from "../types/tileset";

interface Props {
  level: LevelData;
  mode: EditorMode;
  tileTool: TileTool;
  collisionTool: CollisionTool;
  objectTool: ObjectTool;
  tilesets: Map<string, TilesetDef>;
  tilesetImages: Map<string, HTMLImageElement>;
  selectedObject: number;
  selectedPlacement: number;
  mapRevision: number;
  onPointer: (
    worldX: number,
    worldY: number,
    button: number,
    strokeFrom?: { tx: number; ty: number },
  ) => void;
  onStrokeBegin?: () => void;
  onStrokeEnd?: () => void;
  onObjectDrag: (index: number, worldX: number, worldY: number) => void;
  onPlacementDrag: (index: number, cellX: number, cellY: number) => void;
}

const OBJECT_COLORS: Record<string, string> = {
  player: "#dc5a46",
  block: "#78aa5a",
};

type DrawState = {
  camera: { x: number; y: number; zoom: number };
  hover: { tx: number; ty: number } | null;
  level: LevelData;
  mode: EditorMode;
  tileTool: TileTool;
  collisionTool: CollisionTool;
  objectTool: ObjectTool;
  tilesets: Map<string, TilesetDef>;
  tilesetImages: Map<string, HTMLImageElement>;
  selectedObject: number;
  selectedPlacement: number;
};

export function LevelCanvas({
  level,
  mode,
  tileTool,
  collisionTool,
  objectTool,
  tilesets,
  tilesetImages,
  selectedObject,
  selectedPlacement,
  mapRevision,
  onPointer,
  onStrokeBegin,
  onStrokeEnd,
  onObjectDrag,
  onPlacementDrag,
}: Props) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const containerRef = useRef<HTMLDivElement>(null);
  const [camera, setCamera] = useState({ x: 320, y: 240, zoom: 2 });

  const paintingRef = useRef(false);
  const strokeButtonRef = useRef(0);
  const lastStrokeCellRef = useRef<{ tx: number; ty: number } | null>(null);
  const spaceRef = useRef(false);
  const panRef = useRef({ active: false, sx: 0, sy: 0, cx: 0, cy: 0 });
  const dragRef = useRef<
    | { kind: "object"; index: number; offsetX: number; offsetY: number }
    | { kind: "placement"; index: number; offsetX: number; offsetY: number }
    | null
  >(null);
  const hoverRef = useRef<{ tx: number; ty: number } | null>(null);
  const animationStartRef = useRef(performance.now());
  const cameraRef = useRef(camera);
  const drawStateRef = useRef<DrawState>({
    camera,
    hover: null,
    level,
    mode,
    tileTool,
    collisionTool,
    objectTool,
    tilesets,
    tilesetImages,
    selectedObject,
    selectedPlacement,
  });

  cameraRef.current = camera;
  drawStateRef.current = {
    camera,
    hover: hoverRef.current,
    level,
    mode,
    tileTool,
    collisionTool,
    objectTool,
    tilesets,
    tilesetImages,
    selectedObject,
    selectedPlacement,
  };

  // Center camera only when loading a new map — not on every edit.
  useEffect(() => {
    const cell = worldCellSize(level);
    setCamera({
      x: (level.width * cell) / 2,
      y: (level.height * cell) / 2,
      zoom: 2,
    });
  }, [mapRevision]);

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
    const container = containerRef.current;
    if (!container) return;

    const onWheel = (e: WheelEvent) => {
      e.preventDefault();
      e.stopPropagation();
      const factor = e.deltaY < 0 ? 1.1 : 0.9;
      setCamera((c) => ({
        ...c,
        zoom: Math.min(8, Math.max(0.25, c.zoom * factor)),
      }));
    };

    container.addEventListener("wheel", onWheel, { passive: false });
    return () => container.removeEventListener("wheel", onWheel);
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
      const state = drawStateRef.current;
      const { camera: cam, hover, level: lvl } = state;
      const cell = worldCellSize(lvl);
      const rect = container.getBoundingClientRect();
      const w = rect.width;
      const h = rect.height;

      ctx.fillStyle = "#0c0e14";
      ctx.fillRect(0, 0, w, h);

      const toScreen = (wx: number, wy: number) => ({
        x: w * 0.5 + (wx - cam.x) * cam.zoom,
        y: h * 0.5 + (wy - cam.y) * cam.zoom,
      });

      const cellScreen = cell * cam.zoom;
      const elapsedMs = performance.now() - animationStartRef.current;

    for (let y = 0; y < lvl.height; y++) {
        for (let x = 0; x < lvl.width; x++) {
          const p = toScreen(x * cell, y * cell);
          ctx.fillStyle = (x + y) % 2 === 0 ? "#151820" : "#12151c";
          ctx.fillRect(p.x, p.y, cellScreen, cellScreen);
        }
      }

      lvl.tiles.forEach((placement, index) => {
        const frame = animatedTileFrame(placement, elapsedMs);
        const ts = state.tilesets.get(frame.tileset);
        const image = state.tilesetImages.get(frame.tileset);
        const footprintTs = ts ?? state.tilesets.get(placement.tileset);
        const span = footprintTs ? tileCellSpan(footprintTs) : { w: 1, h: 1 };
        const p = toScreen(placement.x * cell, placement.y * cell);
        const pw = span.w * cell * cam.zoom;
        const ph = span.h * cell * cam.zoom;

        if (ts && image) {
          const { col, row } = tileIndexToAtlasCell(frame.id, ts.columns);
          ctx.imageSmoothingEnabled = false;
          ctx.drawImage(
            image,
            col * ts.tile_width,
            row * ts.tile_height,
            ts.tile_width,
            ts.tile_height,
            p.x,
            p.y,
            pw,
            ph,
          );
        } else {
          ctx.fillStyle = "#555";
          ctx.fillRect(p.x, p.y, pw, ph);
        }

        if (index === state.selectedPlacement) {
          ctx.strokeStyle = "#fff6a0";
          ctx.lineWidth = 2;
          ctx.strokeRect(p.x, p.y, pw, ph);
        }
      });

      // Collision overlay — always on top of tiles.
      const collisionFill =
        state.mode === "collision" ? "rgba(255, 50, 50, 0.55)" : "rgba(255, 70, 70, 0.32)";
      const collisionStroke =
        state.mode === "collision" ? "rgba(255, 220, 120, 0.95)" : "rgba(255, 180, 100, 0.55)";

      for (let y = 0; y < lvl.height; y++) {
        for (let x = 0; x < lvl.width; x++) {
          if ((lvl.collision[y]?.[x] ?? 0) === 0) continue;
          const p = toScreen(x * cell, y * cell);
          ctx.fillStyle = collisionFill;
          ctx.fillRect(p.x, p.y, cellScreen, cellScreen);
          ctx.strokeStyle = collisionStroke;
          ctx.lineWidth = state.mode === "collision" ? 2 : 1;
          ctx.strokeRect(p.x + 0.5, p.y + 0.5, cellScreen - 1, cellScreen - 1);
          if (state.mode === "collision" && cellScreen >= 6) {
            ctx.beginPath();
            ctx.moveTo(p.x + 2, p.y + 2);
            ctx.lineTo(p.x + cellScreen - 2, p.y + cellScreen - 2);
            ctx.moveTo(p.x + cellScreen - 2, p.y + 2);
            ctx.lineTo(p.x + 2, p.y + cellScreen - 2);
            ctx.stroke();
          }
        }
      }

      if (hover) {
        const hp = toScreen(hover.tx * cell, hover.ty * cell);
        ctx.strokeStyle =
          state.mode === "collision" ? "rgba(255,200,100,1)" : "rgba(110,168,255,0.9)";
        ctx.lineWidth = 2;
        ctx.strokeRect(hp.x, hp.y, cellScreen, cellScreen);
      }

      lvl.objects.forEach((object, index) => {
        const p = toScreen(object.x, object.y);
        const ow = object.width * cam.zoom;
        const oh = object.height * cam.zoom;
        ctx.fillStyle = OBJECT_COLORS[object.type] ?? "#c8c850";
        ctx.globalAlpha = index === state.selectedObject ? 1 : 0.85;
        ctx.fillRect(p.x, p.y, ow, oh);
        ctx.globalAlpha = 1;
        ctx.strokeStyle = index === state.selectedObject ? "#fff6a0" : "rgba(255,255,255,0.35)";
        ctx.lineWidth = index === state.selectedObject ? 2 : 1;
        ctx.strokeRect(p.x, p.y, ow, oh);
      });

      ctx.fillStyle = "rgba(200,210,230,0.7)";
      ctx.font = "11px JetBrains Mono, monospace";
      const hoverText = hover ? ` · ${hover.tx},${hover.ty}` : "";
      ctx.fillText(
        `${state.mode} · ${Math.round(cam.zoom * 100)}%${hoverText} · MMB/Space pan`,
        12,
        h - 10,
      );

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
    const cell = worldCellSize(level);
    const { x: tx, y: ty } = tileAt(world.x, world.y, cell);
    const prev = hoverRef.current;
    if (!prev || prev.tx !== tx || prev.ty !== ty) {
      hoverRef.current = { tx, ty };
      drawStateRef.current.hover = hoverRef.current;
    }
  };

  const strokeActive =
    (mode === "tiles" && (tileTool === "paint" || tileTool === "erase")) ||
    (mode === "collision" && (collisionTool === "paint" || collisionTool === "erase"));

  const paintStroke = (clientX: number, clientY: number, button: number, isStrokeStart: boolean) => {
    const world = screenToWorld(clientX, clientY);
    const cell = worldCellSize(level);
    const { x: tx, y: ty } = tileAt(world.x, world.y, cell);

    if (isStrokeStart) {
      lastStrokeCellRef.current = { tx, ty };
      onPointer(world.x, world.y, button);
      return;
    }

    const last = lastStrokeCellRef.current;
    if (!last || last.tx !== tx || last.ty !== ty) {
      onPointer(world.x, world.y, button, last ?? undefined);
      lastStrokeCellRef.current = { tx, ty };
    }
  };

  return (
    <div
      ref={containerRef}
      className="canvas-viewport"
      onContextMenu={(e) => e.preventDefault()}
      onPointerDown={(e) => {
        e.preventDefault();
        (e.currentTarget as HTMLDivElement).setPointerCapture(e.pointerId);
        const world = screenToWorld(e.clientX, e.clientY);
        const cell = worldCellSize(level);
        const { x: tx, y: ty } = tileAt(world.x, world.y, cell);

        if (e.button === 1 || (e.button === 0 && spaceRef.current)) {
          panRef.current = {
            active: true,
            sx: e.clientX,
            sy: e.clientY,
            cx: cameraRef.current.x,
            cy: cameraRef.current.y,
          };
          return;
        }

        if (mode === "objects" && objectTool === "select" && e.button === 0) {
          const hit = hitObject(level.objects, world.x, world.y);
          if (hit >= 0) {
            const object = level.objects[hit];
            dragRef.current = {
              kind: "object",
              index: hit,
              offsetX: world.x - object.x,
              offsetY: world.y - object.y,
            };
            onPointer(world.x, world.y, e.button);
            return;
          }
        }

        if (mode === "tiles" && tileTool === "select" && e.button === 0) {
          onPointer(world.x, world.y, e.button);
          return;
        }

        if (strokeActive && e.button === 0) {
          paintingRef.current = true;
          strokeButtonRef.current = e.button;
          onStrokeBegin?.();
          paintStroke(e.clientX, e.clientY, e.button, true);
          return;
        }

        onPointer(world.x, world.y, e.button);
        void tx;
        void ty;
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
        const cell = worldCellSize(level);
        const { x: tx, y: ty } = tileAt(world.x, world.y, cell);

        if (dragRef.current?.kind === "object") {
          onObjectDrag(
            dragRef.current.index,
            world.x - dragRef.current.offsetX,
            world.y - dragRef.current.offsetY,
          );
          return;
        }

        if (dragRef.current?.kind === "placement") {
          onPlacementDrag(
            dragRef.current.index,
            tx - dragRef.current.offsetX,
            ty - dragRef.current.offsetY,
          );
          return;
        }

        if (!paintingRef.current) return;
        const button = e.buttons & 1 ? strokeButtonRef.current : e.button;
        paintStroke(e.clientX, e.clientY, button, false);
      }}
      onPointerUp={(e) => {
        if (paintingRef.current) {
          onStrokeEnd?.();
        }
        paintingRef.current = false;
        lastStrokeCellRef.current = null;
        panRef.current.active = false;
        dragRef.current = null;
        try {
          (e.currentTarget as HTMLDivElement).releasePointerCapture(e.pointerId);
        } catch {
          /* ignore */
        }
      }}
      onPointerLeave={() => {
        if (paintingRef.current) {
          onStrokeEnd?.();
        }
        paintingRef.current = false;
        lastStrokeCellRef.current = null;
        panRef.current.active = false;
        dragRef.current = null;
        hoverRef.current = null;
        drawStateRef.current.hover = null;
      }}
    >
      <canvas ref={canvasRef} />
    </div>
  );
}
