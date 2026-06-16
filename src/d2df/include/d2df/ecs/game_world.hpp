#pragma once

#include <d2df/ecs/components/transform.hpp>
#include <d2df/map/map_document.hpp>
#include <d2df/sim/item_system.hpp>
#include <d2df/sim/map_collision.hpp>
#include <d2df/sim/player_state.hpp>
#include <d2df/sim/projectile_system.hpp>
#include <d2df/sim/shootable_target.hpp>
#include <d2df/sim/trigger_system.hpp>
#include <d2df/sim/weapon_system.hpp>

#include <entt/entt.hpp>
#include <optional>
#include <vector>

namespace d2df {

class EventBus;

constexpr EntityId kPlayerEntityId = 1;
constexpr EntityId kDebugTargetBaseId = 100;

} // namespace d2df

namespace d2df::ecs {

class GameWorld {
public:
    void reset_player(const map::MapDocument& map);
    void fixed_update(const sim::MapCollision& collision, sim::PlayerInput input, EventBus* events,
                      int map_width, int map_height, sim::TriggerSystem* triggers = nullptr);

    [[nodiscard]] sim::PlayerState& player() { return player_; }
    [[nodiscard]] const sim::PlayerState& player() const { return player_; }
    [[nodiscard]] std::vector<sim::ShootableTarget>& targets() { return targets_; }
    [[nodiscard]] const std::vector<sim::ShootableTarget>& targets() const { return targets_; }
    [[nodiscard]] const sim::ProjectileSystem& projectiles() const { return projectiles_; }
    [[nodiscard]] const sim::ItemSystem& items() const { return items_; }
    [[nodiscard]] entt::registry& registry() { return registry_; }
    [[nodiscard]] EntityId player_entity_id() const { return kPlayerEntityId; }

    void respawn_player();

private:
    void sync_to_ecs();
    void spawn_debug_target(const map::MapDocument& map);
    void spawn_map_monsters(const map::MapDocument& map);
    void apply_environment_damage(const sim::MapCollision& collision, EventBus* events);
    void publish_liquid_events(bool was_water, bool was_acid, EventBus* events);
    void publish_landed_event(bool was_on_ground, EventBus* events);

    entt::registry registry_;
    entt::entity player_entity_{entt::null};
    sim::PlayerState player_;
    sim::WeaponSystem weapons_;
    sim::ProjectileSystem projectiles_;
    sim::ItemSystem items_;
    std::vector<sim::ShootableTarget> targets_;
    std::optional<map::MapPoint> spawn_point_;
};

} // namespace d2df::ecs
