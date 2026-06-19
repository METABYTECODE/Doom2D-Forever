#pragma once

#include <rivet/ecs/world.hpp>
#include <rivet/physics/tile_collider.hpp>

namespace rivet::physics {

/// AABB physics: axis-separated move, tile + entity collision, broadphase.
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
    void set_tile_collider(TileCollider collider);
    void set_broadphase_cell_size(float cell_size);

    [[nodiscard]] const Bounds& bounds() const { return bounds_; }
    [[nodiscard]] const TileCollider& tile_collider() const { return tiles_; }

    void step(rivet::ecs::World& world, float fixed_delta_time);
    void clear();

private:
    Bounds bounds_{};
    TileCollider tiles_{};
    float broadphase_cell_size_ = 64.0f;
};

} // namespace rivet::physics
