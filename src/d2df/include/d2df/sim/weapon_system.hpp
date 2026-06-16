#pragma once

#include <d2df/core/types.hpp>
#include <d2df/core/game_events.hpp>
#include <d2df/sim/map_collision.hpp>
#include <d2df/sim/player_state.hpp>
#include <d2df/sim/shootable_target.hpp>

#include <vector>

namespace d2df {

class EventBus;

} // namespace d2df

namespace d2df::sim {

class ProjectileSystem;
class TriggerSystem;

class WeaponSystem {
public:
    void tick(PlayerState& player, const MapCollision& collision, PlayerInput input,
              std::vector<ShootableTarget>& targets, TriggerSystem* triggers,
              ProjectileSystem* projectiles, EntityId shooter_id, EventBus* events, int map_width);

private:
    void try_fire(PlayerState& player, const MapCollision& collision,
                  std::vector<ShootableTarget>& targets, TriggerSystem* triggers,
                  ProjectileSystem* projectiles, EntityId shooter_id, EventBus* events,
                  int map_width, const PlayerInput& input);

    void compute_ray_end(float wx, float wy, float xd, float yd, int map_width, float& ray_x2,
                         float& ray_y2) const;

    void fire_hitscan(PlayerState& player, const MapCollision& collision,
                      std::vector<ShootableTarget>& targets, TriggerSystem* triggers,
                      EntityId shooter_id, EventBus* events, float x0, float y0, float x1,
                      float y1, int damage, WeaponId weapon, bool check_shot_trigger);

    void fire_shotgun(PlayerState& player, const MapCollision& collision,
                      std::vector<ShootableTarget>& targets, TriggerSystem* triggers,
                      EntityId shooter_id, EventBus* events, float wx, float wy, float ray_x2,
                      float ray_y2, int pellet_count, WeaponId weapon);

    void fire_melee(PlayerState& player, std::vector<ShootableTarget>& targets, EntityId shooter_id,
                    EventBus* events, float box_width, float box_height, int damage,
                    events::DamageSource source);

    void publish_player_fired(EventBus* events, EntityId shooter_id, WeaponId weapon, float ox,
                              float oy, float ax, float ay);

    void tick_bfg_charge(PlayerState& player, ProjectileSystem* projectiles, EntityId shooter_id,
                         EventBus* events);
};

} // namespace d2df::sim
