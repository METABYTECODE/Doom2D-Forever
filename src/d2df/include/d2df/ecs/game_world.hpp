#pragma once

#include <d2df/ecs/components/transform.hpp>
#include <d2df/map/map_document.hpp>
#include <d2df/sim/map_collision.hpp>
#include <d2df/sim/player_state.hpp>

#include <entt/entt.hpp>

namespace d2df::ecs {

class GameWorld {
public:
    void reset_player(const map::MapDocument& map);
    void fixed_update(const sim::MapCollision& collision, sim::PlayerInput input);

    [[nodiscard]] sim::PlayerState& player() { return player_; }
    [[nodiscard]] const sim::PlayerState& player() const { return player_; }
    [[nodiscard]] entt::registry& registry() { return registry_; }

private:
    void sync_to_ecs();

    entt::registry registry_;
    entt::entity player_entity_{entt::null};
    sim::PlayerState player_;
};

} // namespace d2df::ecs
