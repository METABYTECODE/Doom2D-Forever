#pragma once

namespace d2df::render {

struct Camera2D {
    float center_x = 0.0f;
    float center_y = 0.0f;
    int scale = 2;

    void pan(float dx, float dy);
    void zoom_in();
    void zoom_out();
    void clamp_scale();

    [[nodiscard]] float screen_to_world_x(float screen_x, int viewport_w) const;
    [[nodiscard]] float screen_to_world_y(float screen_y, int viewport_h) const;

    void world_rect_to_screen(float world_x, float world_y, float world_w, float world_h, int viewport_w,
                              int viewport_h, int& out_x, int& out_y, int& out_w, int& out_h) const;

    /// World-space AABB test against the visible camera region (with optional margin in world units).
    [[nodiscard]] bool world_rect_in_view(float world_x, float world_y, float world_w, float world_h,
                                          int viewport_w, int viewport_h,
                                          float margin_world = 64.0f) const;
};

} // namespace d2df::render
