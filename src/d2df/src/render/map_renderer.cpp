#include <d2df/render/map_renderer.hpp>

#include <SDL.h>

#include <algorithm>

#include <d2df/map/panel_types.hpp>

namespace d2df::render {

void MapRenderer::draw(SDL_Renderer* renderer, const map::MapDocument& document,
                       TextureCache& textures, const Camera2D& camera, int viewport_w,
                       int viewport_h, const MapRenderIndex* panel_index) const {
    for (std::size_t layer_index = 0; layer_index < MapRenderIndex::kLayerCount; ++layer_index) {
        const std::vector<std::size_t>* indices = nullptr;
        if (panel_index != nullptr) {
            indices = &panel_index->layer_panels(layer_index);
        }
        draw_layer(renderer, document, textures, camera, viewport_w, viewport_h,
                   MapRenderIndex::layer_mask(layer_index), indices);
    }
}

void MapRenderer::draw_layer(SDL_Renderer* renderer, const map::MapDocument& document,
                             TextureCache& textures, const Camera2D& camera, int viewport_w,
                             int viewport_h, std::uint16_t mask,
                             const std::vector<std::size_t>* panel_indices) const {
    const auto draw_one = [&](const map::MapPanel& panel) {
        if ((panel.type & mask) == 0) {
            return;
        }
        if ((panel.type & (map::PANEL_CLOSEDOOR | map::PANEL_OPENDOOR)) != 0 && !panel.enabled) {
            return;
        }

        const float panel_x = static_cast<float>(panel.position.x);
        const float panel_y = static_cast<float>(panel.position.y);
        const float panel_w = static_cast<float>(panel.size.width);
        const float panel_h = static_cast<float>(panel.size.height);
        if (!camera.world_rect_in_view(panel_x, panel_y, panel_w, panel_h, viewport_w, viewport_h)) {
            return;
        }

        draw_panel(renderer, document, textures, camera, viewport_w, viewport_h, panel);
    };

    if (panel_indices != nullptr) {
        for (const std::size_t panel_index : *panel_indices) {
            if (panel_index >= document.panels.size()) {
                continue;
            }
            draw_one(document.panels[panel_index]);
        }
        return;
    }

    for (const auto& panel : document.panels) {
        if ((panel.flags & map::PANEL_FLAG_HIDE) != 0) {
            continue;
        }
        draw_one(panel);
    }
}

void MapRenderer::draw_panel(SDL_Renderer* renderer, const map::MapDocument& document,
                             TextureCache& textures, const Camera2D& camera, int viewport_w,
                             int viewport_h, const map::MapPanel& panel) const {
    if (panel.texture >= document.textures.size()) {
        return;
    }

    SDL_Texture* texture = textures.get(document.textures[panel.texture].path);
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

    if (panel.alpha > 0) {
        SDL_SetTextureAlphaMod(texture, static_cast<std::uint8_t>(255 - panel.alpha));
    } else {
        SDL_SetTextureAlphaMod(texture, 255);
    }

    if ((panel.flags & map::PANEL_FLAG_BLENDING) != 0) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_ADD);
    } else {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    }

    const int panel_w = panel.size.width;
    const int panel_h = panel.size.height;
    for (int oy = 0; oy < panel_h; oy += tex_h) {
        const int tile_h = std::min(tex_h, panel_h - oy);
        for (int ox = 0; ox < panel_w; ox += tex_w) {
            const int tile_w = std::min(tex_w, panel_w - ox);

            if (!camera.world_rect_in_view(static_cast<float>(panel.position.x + ox),
                                           static_cast<float>(panel.position.y + oy),
                                           static_cast<float>(tile_w), static_cast<float>(tile_h),
                                           viewport_w, viewport_h)) {
                continue;
            }

            int dst_x = 0;
            int dst_y = 0;
            int dst_w = 0;
            int dst_h = 0;
            camera.world_rect_to_screen(static_cast<float>(panel.position.x + ox),
                                        static_cast<float>(panel.position.y + oy),
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

} // namespace d2df::render
