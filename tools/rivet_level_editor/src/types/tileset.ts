export type TileAnchor = "top-left" | "bottom-left" | "center";

export interface TilesetDef {
  id: string;
  name: string;
  /** Resolved URL for the editor canvas (Vite asset URL). */
  textureUrl: string;
  tile_width: number;
  tile_height: number;
  columns: number;
  rows?: number;
  /** Sprite placement within anchor cell (render only). Default: top-left. */
  anchor?: TileAnchor;
  /** Explicit pixel offset inside anchor cell; overrides anchor when set. */
  offset_x?: number;
  offset_y?: number;
}

export interface SelectedTile {
  tilesetId: string;
  tileId: number;
}
