#pragma once

#include <d2df/ecs/components/transform.hpp>
#include <d2df/engine/ecs/components/collider.hpp>
#include <d2df/engine/map/tile_map.hpp>
#include <d2df/engine/physics/collision_world.hpp>
#include <d2df/engine/runtime/movement_input.hpp>
#include <d2df/sim/player_state.hpp>

#include <entt/entt.hpp>
#include <vector>

namespace d2df::engine {

/// Minimal ECS world for engine v1: player movement + map entities as solid bodies.
class EngineWorld {
public:
    void reset(const map::TileMap& tile_map);
    void fixed_tick(physics::CollisionWorld& collision, MovementInput input);

    [[nodiscard]] entt::registry& registry() { return registry_; }
    [[nodiscard]] const entt::registry& registry() const { return registry_; }

    [[nodiscard]] sim::PlayerState& player() { return player_; }
    [[nodiscard]] const sim::PlayerState& player() const { return player_; }

    [[nodiscard]] entt::entity player_entity() const { return player_entity_; }
    [[nodiscard]] const std::vector<entt::entity>& entity_entities() const {
        return entity_entities_;
    }

private:
    void ensure_player_entity();
    void clear_entities();
    void spawn_entities_from_map(const map::TileMap& tile_map);
    void sync_player_to_ecs();
    void resolve_entity_collisions();

    entt::registry registry_;
    entt::entity player_entity_{entt::null};
    sim::PlayerState player_;
    std::vector<entt::entity> entity_entities_;
};

} // namespace d2df::engine
