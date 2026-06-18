#pragma once

namespace rivet::ecs::components {

struct Collider {
    float width = 32.0f;
    float height = 32.0f;
    float offset_x = 0.0f;
    float offset_y = 0.0f;
    bool is_static = false;
};

} // namespace rivet::ecs::components
