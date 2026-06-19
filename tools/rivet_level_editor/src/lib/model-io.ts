import { defaultAnimation } from "./model-atlas";
import {
  DEFAULT_COLUMNS,
  DEFAULT_FRAME_HEIGHT,
  DEFAULT_FRAME_MS,
  DEFAULT_FRAME_WIDTH,
  MODEL_FORMAT,
  MODEL_VERSION,
  type ModelAnimation,
  type ModelData,
  type ModelFrame,
} from "../types/model";
import { DEFAULT_PACK_ID } from "./resource-pack";

export function createBlankModel(id = "player"): ModelData {
  return {
    format: MODEL_FORMAT,
    version: MODEL_VERSION,
    id,
    name: id,
    resource_pack: DEFAULT_PACK_ID,
    animations: {},
  };
}

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

export function parseModelJson(text: string): ModelData {
  const raw = JSON.parse(text) as Record<string, unknown>;
  const format = String(raw.format ?? "");
  if (format !== MODEL_FORMAT) {
    throw new Error(`Unsupported model format: ${format || "(missing)"}`);
  }

  const version = Number(raw.version ?? 0);
  if (version !== 1 && version !== 2) {
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

  return {
    format: MODEL_FORMAT,
    version: MODEL_VERSION,
    id: String(raw.id ?? "model"),
    name: String(raw.name ?? raw.id ?? "model"),
    resource_pack: String(raw.resource_pack ?? DEFAULT_PACK_ID),
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

export { defaultAnimation };
