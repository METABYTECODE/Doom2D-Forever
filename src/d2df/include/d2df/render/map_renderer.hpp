#pragma once

#include <d2df/map/map_document.hpp>
#include <d2df/render/camera2d.hpp>
#include <d2df/render/map_render_index.hpp>
#include <d2df/render/texture_cache.hpp>

struct SDL_Renderer;

namespace d2df::render {

class MapRenderer {
public:
    void draw(SDL_Renderer* renderer, const map::MapDocument& document, TextureCache& textures,
              const Camera2D& camera, int viewport_w, int viewport_h,
              const MapRenderIndex* panel_index = nullptr) const;

private:
    void draw_layer(SDL_Renderer* renderer, const map::MapDocument& document, TextureCache& textures,
                    const Camera2D& camera, int viewport_w, int viewport_h, std::uint16_t mask,
                    const std::vector<std::size_t>* panel_indices) const;

    void draw_panel(SDL_Renderer* renderer, const map::MapDocument& document, TextureCache& textures,
                    const Camera2D& camera, int viewport_w, int viewport_h,
                    const map::MapPanel& panel) const;
};

} // namespace d2df::render
