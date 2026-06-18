import type { TilesetDef } from "../types/tileset";

const jsonFiles = import.meta.glob("../tilesets/*.json", { eager: true }) as Record<
  string,
  { default: Record<string, unknown> }
>;

const textureFiles = import.meta.glob("../tilesets/*.{png,jpg,jpeg,webp,svg}", {
  eager: true,
  import: "default",
}) as Record<string, string>;

function fileBaseName(path: string): string {
  const name = path.split("/").pop() ?? path;
  return name.replace(/\.[^.]+$/, "");
}

function resolveTextureUrl(textureName: string): string {
  for (const [path, url] of Object.entries(textureFiles)) {
    if (path.endsWith(`/${textureName}`)) {
      return url;
    }
  }
  return "";
}

function parseTileset(path: string, raw: Record<string, unknown>): TilesetDef | null {
  const idFromFile = fileBaseName(path);
  const id = String(raw.id ?? idFromFile);
  const textureName = String(raw.texture ?? `${idFromFile}.png`);
  const textureUrl = resolveTextureUrl(textureName);
  if (!textureUrl) {
    console.warn(`[tilesets] ${id}: texture not found (${textureName})`);
  }

  return {
    id,
    name: String(raw.name ?? id),
    textureUrl,
    tile_width: Number(raw.tile_width ?? 8),
    tile_height: Number(raw.tile_height ?? 8),
    columns: Number(raw.columns ?? 8),
    rows: raw.rows != null ? Number(raw.rows) : undefined,
  };
}

/** Every `*.json` in `src/tilesets/` is one atlas. Pair it with a PNG of the same name. */
export function loadAllTilesets(): Map<string, TilesetDef> {
  const map = new Map<string, TilesetDef>();
  for (const [path, mod] of Object.entries(jsonFiles)) {
    const tileset = parseTileset(path, mod.default);
    if (tileset) {
      map.set(tileset.id, tileset);
    }
  }
  return map;
}

export function loadTilesetImage(tileset: TilesetDef): Promise<HTMLImageElement> {
  return new Promise((resolve, reject) => {
    if (!tileset.textureUrl) {
      reject(new Error(`No texture for tileset: ${tileset.id}`));
      return;
    }
    const image = new Image();
    image.onload = () => resolve(image);
    image.onerror = () => reject(new Error(`Failed to load: ${tileset.id}`));
    image.src = tileset.textureUrl;
  });
}
