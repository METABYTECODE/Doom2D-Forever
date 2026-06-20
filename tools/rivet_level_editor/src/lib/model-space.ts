import {
  DEFAULT_FRAME_HEIGHT,
  DEFAULT_FRAME_WIDTH,
  type ModelAnimation,
  type ModelCollider,
  type ModelData,
  type ModelPivot,
} from "../types/model";

export function referenceFrameSize(model: ModelData): { width: number; height: number } {
  const idle = model.animations.IDLE ?? model.animations.idle ?? Object.values(model.animations)[0];
  if (idle) {
    return { width: idle.frame_width, height: idle.frame_height };
  }
  return { width: DEFAULT_FRAME_WIDTH, height: DEFAULT_FRAME_HEIGHT };
}

/** ECS / physics offset from entity pivot to collider top-left. */
export function colliderOffsetFromPivot(model: ModelData): { offset_x: number; offset_y: number } {
  return {
    offset_x: model.collider.x - model.pivot.x,
    offset_y: model.collider.y - model.pivot.y,
  };
}

export function spriteOffsetFromPivot(model: ModelData): { offset_x: number; offset_y: number } {
  return {
    offset_x: -model.pivot.x,
    offset_y: -model.pivot.y,
  };
}

export function feetPivot(frameWidth: number, frameHeight: number): ModelPivot {
  return { x: frameWidth / 2, y: frameHeight };
}

export function centerPivot(frameWidth: number, frameHeight: number): ModelPivot {
  return { x: frameWidth / 2, y: frameHeight / 2 };
}

/** Blank model: small positive hull inside default reference frame. */
export function blankModelPlacement(): {
  pivot: ModelPivot;
  collider: ModelCollider;
} {
  const width = 32;
  const height = 32;
  return {
    pivot: feetPivot(width, height),
    collider: { x: 6, y: 8, width: 20, height: 24 },
  };
}

/** Typical humanoid hull inside a frame (feet pivot). */
export function humanoidHull(frameWidth: number, frameHeight: number): ModelCollider {
  const width = Math.min(34, frameWidth);
  const height = Math.min(52, frameHeight);
  const pivot = feetPivot(frameWidth, frameHeight);
  return {
    x: pivot.x - width / 2,
    y: pivot.y - height,
    width,
    height,
  };
}

export function fullFrameHull(frameWidth: number, frameHeight: number): ModelCollider {
  return { x: 0, y: 0, width: frameWidth, height: frameHeight };
}

export function applyFeetPivotPreset(model: ModelData, animation?: ModelAnimation): Partial<ModelData> {
  const frame = animation ?? model.animations.IDLE ?? Object.values(model.animations)[0];
  if (!frame) return {};
  return {
    pivot: feetPivot(frame.frame_width, frame.frame_height),
    collider: humanoidHull(frame.frame_width, frame.frame_height),
  };
}
