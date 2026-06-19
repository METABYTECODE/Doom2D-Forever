import type { TilesetDef } from "../types/tileset";

export const DEFAULT_PACK_ID = "dev";

/** Globs under `src/resourcepacks/` (copy of repo pack for the editor). */
const PACK_GLOB_ROOT = "../resourcepacks";

export interface PackManifest {
  id: string;
  name: string;
  version: number;
  description?: string;
}

export interface PackAsset {
  /** Asset id stored in level JSON (e.g. `sky`, `ambient/forest`). */
  id: string;
  label: string;
  category: string;
  url: string;
}

export interface ResourcePackCatalog {
  manifest: PackManifest;
  backgrounds: PackAsset[];
  music: PackAsset[];
  tilesets: Map<string, TilesetDef>;
}

const manifestFiles = import.meta.glob("../resourcepacks/*/pack.json", {
  eager: true,
}) as Record<string, { default: Record<string, unknown> }>;

const backgroundFiles = import.meta.glob(
  "../resourcepacks/*/textures/backgrounds/**/*.{png,jpg,jpeg,webp,gif}",
  { eager: true, import: "default" },
) as Record<string, string>;

const musicFiles = import.meta.glob(
  "../resourcepacks/*/audio/music/**/*.{ogg,wav,mp3,mid,midi}",
  { eager: true, import: "default" },
) as Record<string, string>;

const tilesetJsonFiles = import.meta.glob("../resourcepacks/*/textures/tilesets/**/*.json", {
  eager: true,
}) as Record<string, { default: Record<string, unknown> }>;

const tilesetTextureFiles = import.meta.glob(
  "../resourcepacks/*/textures/tilesets/**/*.{png,jpg,jpeg,webp,svg}",
  { eager: true, import: "default" },
) as Record<string, string>;

function packIdFromPath(path: string): string | null {
  const normalized = path.replace(/\\/g, "/");
  const match = normalized.match(/resourcepacks\/([^/]+)\//);
  return match?.[1] ?? null;
}

function catalogPrefix(packId: string, catalogPath: string): string {
  return `/resourcepacks/${packId}/${catalogPath}/`;
}

function assetIdFromPath(path: string, packId: string, catalogPath: string): string {
  const normalized = path.replace(/\\/g, "/");
  const prefix = catalogPrefix(packId, catalogPath);
  const idx = normalized.indexOf(prefix);
  const relative =
    idx >= 0 ? normalized.slice(idx + prefix.length) : normalized.split("/").pop() ?? normalized;
  return relative.replace(/\.[^.]+$/, "");
}

function assetLabel(id: string): string {
  const parts = id.split("/");
  return parts.length > 1 ? `${parts[parts.length - 2]}/${parts[parts.length - 1]}` : id;
}

function assetCategory(id: string): string {
  const slash = id.lastIndexOf("/");
  return slash < 0 ? "(root)" : id.slice(0, slash);
}

function buildPackAssets(
  files: Record<string, string>,
  packId: string,
  catalogPath: string,
): PackAsset[] {
  const assets: PackAsset[] = [];
  for (const [path, url] of Object.entries(files)) {
    if (packIdFromPath(path) !== packId) continue;
    const id = assetIdFromPath(path, packId, catalogPath);
    assets.push({
      id,
      url,
      label: assetLabel(id),
      category: assetCategory(id),
    });
  }
  assets.sort((a, b) => a.id.localeCompare(b.id));
  return assets;
}

function fileBaseName(path: string): string {
  const name = path.split("/").pop() ?? path;
  return name.replace(/\.[^.]+$/, "");
}

function jsonDirectory(path: string): string {
  const normalized = path.replace(/\\/g, "/");
  return normalized.slice(0, normalized.lastIndexOf("/"));
}

function resolveTilesetTexture(textureName: string, jsonPath: string): string {
  const jsonDir = jsonDirectory(jsonPath);
  for (const [path, url] of Object.entries(tilesetTextureFiles)) {
    const normalized = path.replace(/\\/g, "/");
    if (normalized.endsWith(`/${textureName}`) && normalized.startsWith(jsonDir)) {
      return url;
    }
  }
  for (const [path, url] of Object.entries(tilesetTextureFiles)) {
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
  const textureUrl = resolveTilesetTexture(textureName, path);
  if (!textureUrl) {
    console.warn(`[resourcepack] ${id}: texture not found (${textureName})`);
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

function loadTilesets(packId: string): Map<string, TilesetDef> {
  const map = new Map<string, TilesetDef>();
  for (const [path, mod] of Object.entries(tilesetJsonFiles)) {
    if (packIdFromPath(path) !== packId) continue;
    const tileset = parseTileset(path, mod.default);
    if (!tileset) continue;
    if (map.has(tileset.id)) {
      console.warn(`[resourcepack] duplicate tileset id "${tileset.id}" at ${path}`);
      continue;
    }
    map.set(tileset.id, tileset);
  }
  return map;
}

function loadManifest(packId: string): PackManifest {
  for (const [path, mod] of Object.entries(manifestFiles)) {
    if (packIdFromPath(path) !== packId) continue;
    const raw = mod.default;
    return {
      id: String(raw.id ?? packId),
      name: String(raw.name ?? packId),
      version: Number(raw.version ?? 1),
      description: raw.description != null ? String(raw.description) : undefined,
    };
  }
  return { id: packId, name: packId, version: 1 };
}

export function listPackIds(): string[] {
  const ids = new Set<string>();
  for (const path of Object.keys(manifestFiles)) {
    const id = packIdFromPath(path);
    if (id) ids.add(id);
  }
  return [...ids].sort();
}

export function loadResourcePack(packId: string = DEFAULT_PACK_ID): ResourcePackCatalog {
  return {
    manifest: loadManifest(packId),
    backgrounds: buildPackAssets(backgroundFiles, packId, "textures/backgrounds"),
    music: buildPackAssets(musicFiles, packId, "audio/music"),
    tilesets: loadTilesets(packId),
  };
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

export function loadPackImage(asset: PackAsset | undefined): Promise<HTMLImageElement | null> {
  if (!asset?.url) return Promise.resolve(null);
  return new Promise((resolve) => {
    const image = new Image();
    image.onload = () => resolve(image);
    image.onerror = () => resolve(null);
    image.src = asset.url;
  });
}
