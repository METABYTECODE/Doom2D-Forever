import { LEVEL_FORMAT, LEVEL_VERSION, type LevelData } from "../types/level";

export function validateLevel(level: LevelData): string[] {
  const errors: string[] = [];

  if (level.format !== LEVEL_FORMAT) errors.push("Invalid format id");
  if (level.version !== LEVEL_VERSION) errors.push("Unsupported version");
  if (!level.name.trim()) errors.push("Level name is empty");
  if (level.tile_size < 8) errors.push("Tile size must be at least 8");
  if (level.width < 1 || level.height < 1) errors.push("Map size must be positive");
  if (level.tiles.length !== level.height) {
    errors.push(`Tile rows (${level.tiles.length}) != height (${level.height})`);
  }
  for (let y = 0; y < level.tiles.length; y++) {
    if (level.tiles[y].length !== level.width) {
      errors.push(`Row ${y} width mismatch`);
      break;
    }
  }

  const players = level.objects.filter((o) => o.type === "player");
  if (players.length > 1) errors.push("More than one player object");

  for (const object of level.objects) {
    if (!object.type.trim()) errors.push("Object with empty type");
    if (object.width <= 0 || object.height <= 0) errors.push(`Invalid size for ${object.id || object.type}`);
  }

  return errors;
}
