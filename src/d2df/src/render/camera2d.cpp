#include <d2df/render/camera2d.hpp>

#include <algorithm>
#include <cmath>

namespace d2df::render {

void Camera2D::pan(float dx, float dy) {
    center_x += dx;
    center_y += dy;
}

void Camera2D::zoom_in() {
    scale = std::min(scale + 1, 8);
}

void Camera2D::zoom_out() {
    scale = std::max(scale - 1, 1);
}

void Camera2D::clamp_scale() {
    scale = std::clamp(scale, 1, 8);
}

float Camera2D::screen_to_world_x(float screen_x, int viewport_w) const {
    return center_x + (screen_x - static_cast<float>(viewport_w) * 0.5f) / static_cast<float>(scale);
}

float Camera2D::screen_to_world_y(float screen_y, int viewport_h) const {
    return center_y + (screen_y - static_cast<float>(viewport_h) * 0.5f) / static_cast<float>(scale);
}

void Camera2D::world_rect_to_screen(float world_x, float world_y, float world_w, float world_h,
                                    int viewport_w, int viewport_h, int& out_x, int& out_y, int& out_w,
                                    int& out_h) const {
    const float screen_cx = static_cast<float>(viewport_w) * 0.5f;
    const float screen_cy = static_cast<float>(viewport_h) * 0.5f;
    const float scale_f = static_cast<float>(scale);

    out_x = static_cast<int>(std::lround(screen_cx + (world_x - center_x) * scale_f));
    out_y = static_cast<int>(std::lround(screen_cy + (world_y - center_y) * scale_f));
    out_w = static_cast<int>(std::lround(world_w * scale_f));
    out_h = static_cast<int>(std::lround(world_h * scale_f));
}

bool Camera2D::world_rect_in_view(float world_x, float world_y, float world_w, float world_h,
                                  int viewport_w, int viewport_h, float margin_world) const {
    const float scale_f = static_cast<float>(scale);
    const float half_w =
        static_cast<float>(viewport_w) / (scale_f * 2.0f) + margin_world;
    const float half_h =
        static_cast<float>(viewport_h) / (scale_f * 2.0f) + margin_world;

    const float view_min_x = center_x - half_w;
    const float view_max_x = center_x + half_w;
    const float view_min_y = center_y - half_h;
    const float view_max_y = center_y + half_h;

    const float panel_min_x = world_x;
    const float panel_min_y = world_y;
    const float panel_max_x = world_x + world_w;
    const float panel_max_y = world_y + world_h;

    return panel_max_x >= view_min_x && panel_min_x <= view_max_x &&
           panel_max_y >= view_min_y && panel_min_y <= view_max_y;
}

} // namespace d2df::render
