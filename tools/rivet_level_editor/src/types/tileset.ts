export interface TilesetDef {
  id: string;
  name: string;
  /** Resolved URL for the editor canvas (Vite asset URL). */
  textureUrl: string;
  tile_width: number;
  tile_height: number;
  columns: number;
  rows?: number;
}

export interface SelectedTile {
  tilesetId: string;
  tileId: number;
}
