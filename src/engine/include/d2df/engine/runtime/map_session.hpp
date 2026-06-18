#pragma once

#include <d2df/engine/ecs/engine_world.hpp>
#include <d2df/engine/map/tile_map.hpp>
#include <d2df/engine/physics/collision_world.hpp>

namespace d2df::engine {

/// OOP façade: native tile map + collision + ECS world for engine playtests.
class MapSession {
public:
    void load(map::TileMap tile_map);
    void fixed_tick(MovementInput input);

    [[nodiscard]] const map::TileMap& tile_map() const { return tile_map_; }
    [[nodiscard]] const physics::CollisionWorld& collision() const { return collision_; }
    [[nodiscard]] EngineWorld& world() { return world_; }
    [[nodiscard]] const EngineWorld& world() const { return world_; }

private:
    map::TileMap tile_map_;
    physics::CollisionWorld collision_;
    EngineWorld world_;
};

} // namespace d2df::engine
