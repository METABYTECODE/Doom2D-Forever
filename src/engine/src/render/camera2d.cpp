#include <algorithm>

#include <rivet/render/camera2d.hpp>

namespace rivet::render {

namespace {

[[nodiscard]] float clamp01(const float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

} // namespace

void Camera2D::set_viewport(const int width, const int height) {
    viewport_width = width;
    viewport_height = height;
}

void Camera2D::center_on(const float world_x, const float world_y) {
    x = world_x;
    y = world_y;
}

void Camera2D::follow(const float target_x, const float target_y, const float smooth) {
    const float t = clamp01(smooth);
    x += (target_x - x) * t;
    y += (target_y - y) * t;
}

Rect Camera2D::world_to_screen(const Rect& world_rect) const {
  const float screen_center_x = static_cast<float>(viewport_width) * 0.5f;
  const float screen_center_y = static_cast<float>(viewport_height) * 0.5f;

  return Rect{
      screen_center_x + (world_rect.x - x) * zoom,
      screen_center_y + (world_rect.y - y) * zoom,
      world_rect.width * zoom,
      world_rect.height * zoom,
  };
}

Rect Camera2D::screen_to_world(const Rect& screen_rect) const {
    const float screen_center_x = static_cast<float>(viewport_width) * 0.5f;
    const float screen_center_y = static_cast<float>(viewport_height) * 0.5f;

    return Rect{
        x + (screen_rect.x - screen_center_x) / zoom,
        y + (screen_rect.y - screen_center_y) / zoom,
        screen_rect.width / zoom,
        screen_rect.height / zoom,
    };
}

} // namespace rivet::render
