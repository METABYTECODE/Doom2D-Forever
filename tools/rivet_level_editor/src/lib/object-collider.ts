import type { LevelObject } from "../types/level";
import type { ModelCollider, ModelData } from "../types/model";
import { colliderOffsetFromPivot } from "./model-space";
import { blankModelPlacement } from "./model-space";

export function resolveModelId(object: LevelObject): string | undefined {
  if (object.model) return object.model;
  if (object.type === "player") return "player";
  return undefined;
}

export function modelColliderForObject(
  object: LevelObject,
  models: ReadonlyMap<string, ModelData>,
): ModelCollider {
  const modelId = resolveModelId(object);
  if (modelId) {
    const model = models.get(modelId);
    if (model) return model.collider;
  }
  if (object.type === "player") {
    return blankModelPlacement().collider;
  }
  const width = object.width ?? 48;
  const height = object.height ?? 48;
  return { x: 0, y: 0, width, height };
}

/** World-space physics AABB. Object x,y is entity pivot. */
export function objectColliderAabb(
  object: LevelObject,
  models: ReadonlyMap<string, ModelData>,
): { x: number; y: number; width: number; height: number } {
  const modelId = resolveModelId(object);
  if (modelId) {
    const model = models.get(modelId);
    if (model) {
      const offset = colliderOffsetFromPivot(model);
      return {
        x: object.x + offset.offset_x,
        y: object.y + offset.offset_y,
        width: model.collider.width,
        height: model.collider.height,
      };
    }
  }

  if (object.type === "player") {
    const blank = blankModelPlacement();
    const offset = {
      offset_x: blank.collider.x - blank.pivot.x,
      offset_y: blank.collider.y - blank.pivot.y,
    };
    return {
      x: object.x + offset.offset_x,
      y: object.y + offset.offset_y,
      width: blank.collider.width,
      height: blank.collider.height,
    };
  }

  const collider = modelColliderForObject(object, models);
  return {
    x: object.x + collider.x,
    y: object.y + collider.y,
    width: collider.width,
    height: collider.height,
  };
}

export function defaultPlayerCollider(models: ReadonlyMap<string, ModelData>): ModelCollider {
  const player = models.get("player");
  if (player) return { ...player.collider };
  return { ...blankModelPlacement().collider };
}

export function playerPlacementPivot(
  snapX: number,
  snapY: number,
  gridSize: number,
): { x: number; y: number } {
  return { x: snapX + gridSize / 2, y: snapY + gridSize };
}

export function colliderAabbFromPivot(
  pivotX: number,
  pivotY: number,
  model: ModelData,
): { x: number; y: number; width: number; height: number } {
  const offset = colliderOffsetFromPivot(model);
  return {
    x: pivotX + offset.offset_x,
    y: pivotY + offset.offset_y,
    width: model.collider.width,
    height: model.collider.height,
  };
}

export function defaultPlayerPivot(
  models: ReadonlyMap<string, ModelData>,
  gridSize: number,
  snapX: number,
  snapY: number,
): { x: number; y: number } {
  return playerPlacementPivot(snapX, snapY, gridSize);
}
