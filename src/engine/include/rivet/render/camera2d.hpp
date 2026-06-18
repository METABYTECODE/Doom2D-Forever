#pragma once

#include <rivet/render/rect.hpp>

namespace rivet::render {

/// Simple 2D camera: world center + zoom mapped into a viewport.
class Camera2D {
public:
    float x = 0.0f;
    float y = 0.0f;
    float zoom = 1.0f;
    int viewport_width = 800;
    int viewport_height = 600;

    void set_viewport(int width, int height);
    void center_on(float world_x, float world_y);
    void follow(float target_x, float target_y, float smooth = 0.15f);

    [[nodiscard]] Rect world_to_screen(const Rect& world_rect) const;
    [[nodiscard]] Rect screen_to_world(const Rect& screen_rect) const;
};

} // namespace rivet::render
