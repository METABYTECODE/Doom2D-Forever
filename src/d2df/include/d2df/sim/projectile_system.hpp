#pragma once

#include <d2df/core/types.hpp>
#include <d2df/sim/map_collision.hpp>
#include <d2df/sim/player_state.hpp>
#include <d2df/sim/shootable_target.hpp>
#include <d2df/sim/weapon_types.hpp>

#include <vector>

namespace d2df {

class EventBus;

} // namespace d2df

namespace d2df::sim {

class TriggerSystem;

struct Projectile {
    bool active = false;
    WeaponId weapon = WeaponId::RocketLauncher;
    EntityId shooter_id = 0;
    float x = 0.0f;
    float y = 0.0f;
    float prev_x = 0.0f;
    float prev_y = 0.0f;
    float width = 14.0f;
    float height = 14.0f;
    int vel_x = 0;
    int vel_y = 0;
    int timeout = 0;
    int spawn_grace_ticks = 0;
};

class ProjectileSystem {
public:
    void clear();

    void spawn_rocket(float x, float y, float aim_x, float aim_y, EntityId shooter_id);
    void spawn_plasma(float x, float y, float aim_x, float aim_y, EntityId shooter_id);
    void spawn_bfg(float x, float y, float aim_x, float aim_y, EntityId shooter_id);
    void spawn_monster_shot(float x, float y, float aim_x, float aim_y, EntityId shooter_id);

    void tick(const MapCollision& collision, TriggerSystem* triggers, PlayerState& player,
              std::vector<ShootableTarget>& targets, EventBus* events, int map_width,
              int map_height);

    [[nodiscard]] const std::vector<Projectile>& projectiles() const { return projectiles_; }

private:
    std::size_t allocate_slot();
    void launch_projectile(std::size_t index, float x, float y, float aim_x, float aim_y,
                           int speed, int lifetime_ticks);
    void destroy_projectile(std::size_t index);
    void explode_rocket(std::size_t index, TriggerSystem* triggers, PlayerState& player,
                        std::vector<ShootableTarget>& targets, EventBus* events);
    void explode_bfg(std::size_t index, TriggerSystem* triggers, PlayerState& player,
                     std::vector<ShootableTarget>& targets, EventBus* events);
    void tick_projectile(std::size_t index, const MapCollision& collision, TriggerSystem* triggers,
                         PlayerState& player, std::vector<ShootableTarget>& targets,
                         EventBus* events, int map_width, int map_height);

    std::vector<Projectile> projectiles_;
};

} // namespace d2df::sim
