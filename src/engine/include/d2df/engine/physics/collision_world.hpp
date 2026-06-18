#pragma once

#include <d2df/engine/map/tile_map.hpp>
#include <d2df/sim/map_collision.hpp>

namespace d2df::engine::physics {

/// Tile-map collision facade. Builds legacy collision data from native tiles.
class CollisionWorld {
public:
    void build(const map::TileMap& tile_map);

    [[nodiscard]] const sim::MapCollision& collision() const { return collision_; }

private:
    sim::MapCollision collision_;
};

} // namespace d2df::engine::physics
