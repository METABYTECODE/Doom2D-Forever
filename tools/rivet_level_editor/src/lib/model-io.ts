import { defaultAnimation } from "./model-atlas";
import {
  applyFeetPivotPreset,
  blankModelPlacement,
  feetPivot,
  fullFrameHull,
  humanoidHull,
} from "./model-space";
import {
  DEFAULT_COLUMNS,
  DEFAULT_FRAME_HEIGHT,
  DEFAULT_FRAME_MS,
  DEFAULT_FRAME_WIDTH,
  MODEL_FORMAT,
  MODEL_VERSION,
  type ModelAnimation,
  type ModelCollider,
  type ModelData,
  type ModelFrame,
  type ModelPivot,
} from "../types/model";
import { DEFAULT_PACK_ID } from "./resource-pack";

function parseFrame(raw: Record<string, unknown>): ModelFrame {
  return { id: Number(raw.id ?? 0) };
}

function parseAnimation(raw: Record<string, unknown>): ModelAnimation {
  const frames = Array.isArray(raw.frames)
    ? raw.frames.map((frame) => parseFrame(frame as Record<string, unknown>))
    : [{ id: 0 }];

  const atlas = String(raw.atlas ?? raw.sprite ?? "");

  return {
    atlas,
    frame_width: Number(raw.frame_width ?? DEFAULT_FRAME_WIDTH),
    frame_height: Number(raw.frame_height ?? DEFAULT_FRAME_HEIGHT),
    columns: Number(raw.columns ?? DEFAULT_COLUMNS),
    frames,
    frame_ms: raw.frame_ms != null ? Number(raw.frame_ms) : DEFAULT_FRAME_MS,
    loop: raw.loop !== false,
  };
}

function parsePivot(raw: Record<string, unknown> | undefined, frameW: number, frameH: number): ModelPivot {
  if (raw) {
    return { x: Number(raw.x ?? frameW / 2), y: Number(raw.y ?? frameH) };
  }
  return feetPivot(frameW, frameH);
}

function parseColliderRect(
  raw: Record<string, unknown>,
  pivot: ModelPivot,
  frameW: number,
  frameH: number,
): ModelCollider {
  if ("offset_x" in raw || "offset_y" in raw) {
    const width = Number(raw.width ?? frameW);
    const height = Number(raw.height ?? frameH);
    const offsetX = Number(raw.offset_x ?? -width / 2);
    const offsetY = Number(raw.offset_y ?? -height);
    return {
      x: pivot.x + offsetX,
      y: pivot.y + offsetY,
      width,
      height,
    };
  }

  return {
    x: Number(raw.x ?? 0),
    y: Number(raw.y ?? 0),
    width: Number(raw.width ?? frameW),
    height: Number(raw.height ?? frameH),
  };
}

function migrateDeprecatedHitbox(
  raw: Record<string, unknown>,
  animations: Record<string, ModelAnimation>,
): { pivot: ModelPivot; collider: ModelCollider } | null {
  if (!raw.hitbox || typeof raw.hitbox !== "object") return null;
  const hb = raw.hitbox as Record<string, unknown>;
  const idle = animations.IDLE ?? animations.idle ?? Object.values(animations)[0];
  const frameW = idle?.frame_width ?? DEFAULT_FRAME_WIDTH;
  const frameH = idle?.frame_height ?? DEFAULT_FRAME_HEIGHT;
  const pivot = feetPivot(frameW, frameH);
  const width = Number(hb.width ?? 34);
  const height = Number(hb.height ?? 52);
  const originX = Number(hb.origin_x ?? 0);
  const originY = Number(hb.origin_y ?? 0);
  return {
    pivot,
    collider: { x: originX, y: originY, width, height },
  };
}

function inferBody(
  raw: Record<string, unknown>,
  animations: Record<string, ModelAnimation>,
): { pivot: ModelPivot; collider: ModelCollider } {
  const migratedHitbox = migrateDeprecatedHitbox(raw, animations);
  if (migratedHitbox) return migratedHitbox;

  const idle = animations.IDLE ?? animations.idle ?? Object.values(animations)[0];
  const frameW = idle?.frame_width ?? DEFAULT_FRAME_WIDTH;
  const frameH = idle?.frame_height ?? DEFAULT_FRAME_HEIGHT;

  const pivotRaw =
    raw.pivot && typeof raw.pivot === "object" ? (raw.pivot as Record<string, unknown>) : undefined;
  const pivot = parsePivot(pivotRaw, frameW, frameH);

  if (raw.collider && typeof raw.collider === "object") {
    return {
      pivot,
      collider: parseColliderRect(raw.collider as Record<string, unknown>, pivot, frameW, frameH),
    };
  }

  if (idle) {
    return { pivot: feetPivot(frameW, frameH), collider: humanoidHull(frameW, frameH) };
  }

  return blankModelPlacement();
}

export function createBlankModel(id = "player"): ModelData {
  const { pivot, collider } = blankModelPlacement();
  return {
    format: MODEL_FORMAT,
    version: MODEL_VERSION,
    id,
    name: id,
    resource_pack: DEFAULT_PACK_ID,
    pivot,
    collider,
    animations: {},
  };
}

export function parseModelJson(text: string): ModelData {
  const raw = JSON.parse(text) as Record<string, unknown>;
  const format = String(raw.format ?? "");
  if (format !== MODEL_FORMAT) {
    throw new Error(`Unsupported model format: ${format || "(missing)"}`);
  }

  const version = Number(raw.version ?? 0);
  if (version !== 1 && version !== 2 && version !== 3) {
    throw new Error(`Unsupported model version: ${version}`);
  }

  const animations: Record<string, ModelAnimation> = {};
  if (raw.animations && typeof raw.animations === "object") {
    for (const [key, value] of Object.entries(raw.animations)) {
      if (value && typeof value === "object") {
        animations[key] = parseAnimation(value as Record<string, unknown>);
      }
    }
  }

  const body = inferBody(raw, animations);

  return {
    format: MODEL_FORMAT,
    version: MODEL_VERSION,
    id: String(raw.id ?? "model"),
    name: String(raw.name ?? raw.id ?? "model"),
    resource_pack: String(raw.resource_pack ?? DEFAULT_PACK_ID),
    pivot: body.pivot,
    collider: body.collider,
    animations,
  };
}

export function serializeModel(model: ModelData): string {
  const animations: Record<string, unknown> = {};
  const keys = Object.keys(model.animations).sort();
  for (const key of keys) {
    const anim = model.animations[key];
    animations[key] = {
      atlas: anim.atlas,
      frame_width: anim.frame_width,
      frame_height: anim.frame_height,
      columns: anim.columns,
      frames: anim.frames.map((frame) => ({ id: frame.id })),
      frame_ms: anim.frame_ms,
      loop: anim.loop,
    };
  }

  const payload = {
    format: model.format,
    version: model.version,
    id: model.id,
    name: model.name,
    resource_pack: model.resource_pack,
    pivot: { x: model.pivot.x, y: model.pivot.y },
    collider: {
      x: model.collider.x,
      y: model.collider.y,
      width: model.collider.width,
      height: model.collider.height,
    },
    animations,
  };
  return JSON.stringify(payload, null, 2);
}

export function downloadModel(model: ModelData, filename?: string): void {
  const blob = new Blob([serializeModel(model)], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement("a");
  anchor.href = url;
  anchor.download = filename ?? `${model.id}.model.json`;
  anchor.click();
  URL.revokeObjectURL(url);
}

export { defaultAnimation, applyFeetPivotPreset, fullFrameHull };
