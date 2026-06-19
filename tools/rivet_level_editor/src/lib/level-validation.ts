import { FLUID_NONE, LEVEL_FORMAT, LEVEL_VERSION, type LevelData } from "../types/level";
import { subGridCols, subGridRows, snapGridSize } from "./sub-grid";

export function validateLevel(level: LevelData): string[] {
  const errors: string[] = [];
  const gridSize = snapGridSize(level);
  const cols = subGridCols(level.width, gridSize);
  const rows = subGridRows(level.height, gridSize);

  if (level.format !== LEVEL_FORMAT) errors.push("Invalid format id");
  if (level.version !== LEVEL_VERSION) errors.push("Unsupported version");
  if (!level.name.trim()) errors.push("Level name is empty");
  if (level.grid_size < 1) errors.push("grid_size must be positive");
  if (level.width < 1 || level.height < 1) errors.push("Map size must be positive");
  if (level.collision.length !== rows) {
    errors.push(`Collision rows (${level.collision.length}) != sub-grid rows (${rows})`);
  }
  for (let y = 0; y < level.collision.length; y++) {
    if (level.collision[y].length !== cols) {
      errors.push(`Collision row ${y} width mismatch (expected ${cols})`);
      break;
    }
  }
  if (level.fluids.length !== rows) {
    errors.push(`Fluids rows (${level.fluids.length}) != sub-grid rows (${rows})`);
  }
  for (let y = 0; y < level.fluids.length; y++) {
    if (level.fluids[y].length !== cols) {
      errors.push(`Fluids row ${y} width mismatch (expected ${cols})`);
      break;
    }
    for (let x = 0; x < level.fluids[y].length; x++) {
      const cell = level.fluids[y][x];
      if (cell < FLUID_NONE || cell > 3) {
        errors.push(`Invalid fluid value ${cell} at ${x},${y}`);
        break;
      }
    }
  }

  const players = level.objects.filter((o) => o.type === "player");
  if (players.length > 1) errors.push("More than one player object");

  for (const tile of level.tiles) {
    if (!tile.tileset.trim()) errors.push("Placed tile with empty tileset id");
    if (tile.x < 0 || tile.y < 0) errors.push("Placed tile with negative coordinates");
    if (tile.frames) {
      for (const frame of tile.frames) {
        if (!frame.tileset.trim()) errors.push("Animated tile frame with empty tileset id");
      }
      if (tile.frames.length === 1) errors.push("Animated tile has only one frame");
    }
    if (tile.frame_ms != null && tile.frame_ms <= 0) {
      errors.push("frame_ms must be positive");
    }
  }

  for (const object of level.objects) {
    if (!object.type.trim()) errors.push("Object with empty type");
    if (object.width <= 0 || object.height <= 0) errors.push(`Invalid size for ${object.id || object.type}`);
  }

  return errors;
}
