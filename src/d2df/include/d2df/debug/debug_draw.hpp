#pragma once

#include <d2df/render/camera2d.hpp>

#include <cstdint>

struct SDL_Renderer;

namespace d2df::debug {

void draw_world_rect_outline(SDL_Renderer* renderer, const render::Camera2D& camera, int viewport_w,
                             int viewport_h, float world_x, float world_y, float world_w,
                             float world_h, std::uint8_t r, std::uint8_t g, std::uint8_t b,
                             std::uint8_t a = 220);

void draw_world_rect_fill(SDL_Renderer* renderer, const render::Camera2D& camera, int viewport_w,
                          int viewport_h, float world_x, float world_y, float world_w,
                          float world_h, std::uint8_t r, std::uint8_t g, std::uint8_t b,
                          std::uint8_t a = 80);

} // namespace d2df::debug
