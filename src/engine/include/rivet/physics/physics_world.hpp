#pragma once

#include <rivet/ecs/world.hpp>

namespace rivet::physics {

/// Simple AABB physics for engine demos. Game-specific solvers stay in the game layer.
class PhysicsWorld {
public:
    struct Bounds {
        float min_x = 0.0f;
        float min_y = 0.0f;
        float max_x = 0.0f;
        float max_y = 0.0f;
        bool enabled = false;
    };

    void set_world_bounds(const Bounds& bounds);
    void step(ecs::World& world, float fixed_delta_time);
    void clear();

private:
    Bounds bounds_{};
};

} // namespace rivet::physics
