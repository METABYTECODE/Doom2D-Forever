#pragma once

#include <d2df/engine/map/tile_map.hpp>
#include <d2df/engine/render/tile_map_render_index.hpp>
#include <d2df/render/camera2d.hpp>
#include <d2df/render/texture_cache.hpp>

struct SDL_Renderer;

namespace d2df::engine::render {

class TileMapRenderer {
public:
    void draw(SDL_Renderer* renderer, const map::TileMap& tile_map, ::d2df::render::TextureCache& textures,
              const ::d2df::render::Camera2D& camera, int viewport_w, int viewport_h,
              const TileMapRenderIndex* tile_index = nullptr) const;

private:
    void draw_layer(SDL_Renderer* renderer, const map::TileMap& tile_map,
                    ::d2df::render::TextureCache& textures, const ::d2df::render::Camera2D& camera,
                    int viewport_w, int viewport_h, std::uint32_t mask,
                    const std::vector<TileDrawRef>* tile_refs) const;

    void draw_tile(SDL_Renderer* renderer, const map::TileMap& tile_map,
                   ::d2df::render::TextureCache& textures, const ::d2df::render::Camera2D& camera,
                   int viewport_w, int viewport_h, const map::TilePlacement& tile) const;
};

} // namespace d2df::engine::render
