#include <d2df/engine/render/tile_map_renderer.hpp>

#include <SDL.h>

#include <algorithm>

#include <d2df/engine/map/tile_flags.hpp>

namespace d2df::engine::render {

void TileMapRenderer::draw(SDL_Renderer* renderer, const map::TileMap& tile_map,
                           ::d2df::render::TextureCache& textures,
                           const ::d2df::render::Camera2D& camera, int viewport_w, int viewport_h,
                           const TileMapRenderIndex* tile_index) const {
    for (std::size_t layer_index = 0; layer_index < TileMapRenderIndex::kLayerCount; ++layer_index) {
        const std::vector<TileDrawRef>* refs = nullptr;
        if (tile_index != nullptr) {
            refs = &tile_index->layer_tiles(layer_index);
        }
        draw_layer(renderer, tile_map, textures, camera, viewport_w, viewport_h,
                   TileMapRenderIndex::layer_mask(layer_index), refs);
    }
}

void TileMapRenderer::draw_layer(SDL_Renderer* renderer, const map::TileMap& tile_map,
                                 ::d2df::render::TextureCache& textures,
                                 const ::d2df::render::Camera2D& camera, int viewport_w,
                                 int viewport_h, std::uint32_t mask,
                                 const std::vector<TileDrawRef>* tile_refs) const {
    const auto draw_one = [&](const map::TilePlacement& tile) {
        if ((tile.flags & mask) == 0) {
            return;
        }
        if ((tile.flags & (map::TILE_CLOSE_DOOR | map::TILE_OPEN_DOOR)) != 0 && !tile.enabled) {
            return;
        }

        const float tile_x = static_cast<float>(tile.x);
        const float tile_y = static_cast<float>(tile.y);
        const float tile_w = static_cast<float>(tile.width);
        const float tile_h = static_cast<float>(tile.height);
        if (!camera.world_rect_in_view(tile_x, tile_y, tile_w, tile_h, viewport_w, viewport_h)) {
            return;
        }

        draw_tile(renderer, tile_map, textures, camera, viewport_w, viewport_h, tile);
    };

    if (tile_refs != nullptr) {
        for (const TileDrawRef& ref : *tile_refs) {
            if (ref.layer_index >= tile_map.layers.size()) {
                continue;
            }
            const auto& layer = tile_map.layers[ref.layer_index];
            if (ref.tile_index >= layer.tiles.size()) {
                continue;
            }
            draw_one(layer.tiles[ref.tile_index]);
        }
        return;
    }

    for (const auto& layer : tile_map.layers) {
        for (const auto& tile : layer.tiles) {
            if ((tile.visibility & map::TILE_VIS_HIDDEN) != 0) {
                continue;
            }
            draw_one(tile);
        }
    }
}

void TileMapRenderer::draw_tile(SDL_Renderer* renderer, const map::TileMap& tile_map,
                                ::d2df::render::TextureCache& textures,
                                const ::d2df::render::Camera2D& camera, int viewport_w,
                                int viewport_h, const map::TilePlacement& tile) const {
    if (tile.tile_index >= tile_map.tileset.size()) {
        return;
    }

    SDL_Texture* texture = textures.get(tile_map.tileset[tile.tile_index].asset_id);
    if (texture == nullptr) {
        return;
    }

    int tex_w = 0;
    int tex_h = 0;
    if (!textures.query_size(texture, tex_w, tex_h)) {
        SDL_QueryTexture(texture, nullptr, nullptr, &tex_w, &tex_h);
    }
    if (tex_w <= 0 || tex_h <= 0) {
        return;
    }

    if (tile.alpha > 0) {
        SDL_SetTextureAlphaMod(texture, static_cast<std::uint8_t>(255 - tile.alpha));
    } else {
        SDL_SetTextureAlphaMod(texture, 255);
    }

    if ((tile.visibility & map::TILE_VIS_BLEND) != 0) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_ADD);
    } else {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    }

    const int panel_w = tile.width;
    const int panel_h = tile.height;
    for (int oy = 0; oy < panel_h; oy += tex_h) {
        const int tile_h = std::min(tex_h, panel_h - oy);
        for (int ox = 0; ox < panel_w; ox += tex_w) {
            const int tile_w = std::min(tex_w, panel_w - ox);

            if (!camera.world_rect_in_view(static_cast<float>(tile.x + ox),
                                           static_cast<float>(tile.y + oy),
                                           static_cast<float>(tile_w), static_cast<float>(tile_h),
                                           viewport_w, viewport_h)) {
                continue;
            }

            int dst_x = 0;
            int dst_y = 0;
            int dst_w = 0;
            int dst_h = 0;
            camera.world_rect_to_screen(static_cast<float>(tile.x + ox), static_cast<float>(tile.y + oy),
                                        static_cast<float>(tile_w), static_cast<float>(tile_h),
                                        viewport_w, viewport_h, dst_x, dst_y, dst_w, dst_h);

            if (dst_w <= 0 || dst_h <= 0) {
                continue;
            }

            const SDL_Rect src{0, 0, tile_w, tile_h};
            const SDL_Rect dst{dst_x, dst_y, dst_w, dst_h};
            SDL_RenderCopy(renderer, texture, &src, &dst);
        }
    }
}

} // namespace d2df::engine::render
