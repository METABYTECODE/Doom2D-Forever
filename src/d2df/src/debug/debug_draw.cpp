#include <d2df/debug/debug_draw.hpp>

#include <SDL.h>

namespace d2df::debug {

void draw_world_rect_outline(SDL_Renderer* renderer, const render::Camera2D& camera, int viewport_w,
                             int viewport_h, float world_x, float world_y, float world_w,
                             float world_h, std::uint8_t r, std::uint8_t g, std::uint8_t b,
                             std::uint8_t a) {
    int dst_x = 0;
    int dst_y = 0;
    int dst_w = 0;
    int dst_h = 0;
    camera.world_rect_to_screen(world_x, world_y, world_w, world_h, viewport_w, viewport_h, dst_x,
                                dst_y, dst_w, dst_h);
    if (dst_w <= 0 || dst_h <= 0) {
        return;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    const SDL_Rect rect{dst_x, dst_y, dst_w, dst_h};
    SDL_RenderDrawRect(renderer, &rect);
}

void draw_world_rect_fill(SDL_Renderer* renderer, const render::Camera2D& camera, int viewport_w,
                          int viewport_h, float world_x, float world_y, float world_w,
                          float world_h, std::uint8_t r, std::uint8_t g, std::uint8_t b,
                          std::uint8_t a) {
    int dst_x = 0;
    int dst_y = 0;
    int dst_w = 0;
    int dst_h = 0;
    camera.world_rect_to_screen(world_x, world_y, world_w, world_h, viewport_w, viewport_h, dst_x,
                                dst_y, dst_w, dst_h);
    if (dst_w <= 0 || dst_h <= 0) {
        return;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    const SDL_Rect rect{dst_x, dst_y, dst_w, dst_h};
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, r, g, b, 220);
    SDL_RenderDrawRect(renderer, &rect);
}

} // namespace d2df::debug
