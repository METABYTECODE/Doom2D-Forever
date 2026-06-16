#pragma once

#include <d2df/ecs/components/transform.hpp>
#include <d2df/map/map_document.hpp>
#include <d2df/sim/map_collision.hpp>
#include <d2df/sim/player_state.hpp>

#include <entt/entt.hpp>
#include <optional>

namespace d2df {

class EventBus;

} // namespace d2df

namespace d2df::ecs {

class GameWorld {
public:
    void reset_player(const map::MapDocument& map);
    void fixed_update(const sim::MapCollision& collision, sim::PlayerInput input, EventBus* events);

    [[nodiscard]] sim::PlayerState& player() { return player_; }
    [[nodiscard]] const sim::PlayerState& player() const { return player_; }
    [[nodiscard]] entt::registry& registry() { return registry_; }

    void respawn_player();

private:
    void sync_to_ecs();
    void apply_environment_damage(const sim::MapCollision& collision, EventBus* events);
    void publish_liquid_events(bool was_water, bool was_acid, EventBus* events);
    void publish_landed_event(bool was_on_ground, EventBus* events);

    entt::registry registry_;
    entt::entity player_entity_{entt::null};
    sim::PlayerState player_;
    std::optional<map::MapPoint> spawn_point_;
};

} // namespace d2df::ecs
