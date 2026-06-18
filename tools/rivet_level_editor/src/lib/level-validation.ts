import { LEVEL_FORMAT, LEVEL_VERSION, type LevelData } from "../types/level";

export function validateLevel(level: LevelData): string[] {
  const errors: string[] = [];

  if (level.format !== LEVEL_FORMAT) errors.push("Invalid format id");
  if (level.version !== LEVEL_VERSION) errors.push("Unsupported version");
  if (!level.name.trim()) errors.push("Level name is empty");
  if (level.grid_size < 1) errors.push("grid_size must be positive");
  if (level.width < 1 || level.height < 1) errors.push("Map size must be positive");
  if (level.collision.length !== level.height) {
    errors.push(`Collision rows (${level.collision.length}) != height (${level.height})`);
  }
  for (let y = 0; y < level.collision.length; y++) {
    if (level.collision[y].length !== level.width) {
      errors.push(`Collision row ${y} width mismatch`);
      break;
    }
  }

  const players = level.objects.filter((o) => o.type === "player");
  if (players.length > 1) errors.push("More than one player object");

  for (const tile of level.tiles) {
    if (!tile.tileset.trim()) errors.push("Placed tile with empty tileset id");
    if (tile.x < 0 || tile.y < 0) errors.push("Placed tile with negative coordinates");
  }

  for (const object of level.objects) {
    if (!object.type.trim()) errors.push("Object with empty type");
    if (object.width <= 0 || object.height <= 0) errors.push(`Invalid size for ${object.id || object.type}`);
  }

  return errors;
}
