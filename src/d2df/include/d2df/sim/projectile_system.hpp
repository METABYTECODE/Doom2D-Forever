#pragma once

#include <d2df/core/types.hpp>
#include <d2df/map/monster_types.hpp>
#include <d2df/sim/map_collision.hpp>
#include <d2df/sim/player_state.hpp>
#include <d2df/sim/shootable_target.hpp>
#include <d2df/sim/weapon_types.hpp>

#include <vector>

namespace d2df {

class EventBus;

} // namespace d2df

namespace d2df::sim {

enum class ProjectileKind : std::uint8_t {
    Bullet = 0,
    ShotgunPellet,
    Plasma,
    Rocket,
    Bfg,
    ImpFire,
    CacoFire,
    BaronFire,
    BspFire,
    SkelFire,
    MancubFire,
    Flame,
};

struct Projectile {
    bool active = false;
    ProjectileKind kind = ProjectileKind::Bullet;
    WeaponId weapon = WeaponId::Pistol;
    EntityId shooter_id = 0;
    float x = 0.0f;
    float y = 0.0f;
    float prev_x = 0.0f;
    float prev_y = 0.0f;
    float width = 14.0f;
    float height = 14.0f;
    int vel_x = 0;
    int vel_y = 0;
    int damage = 3;
    int timeout = 0;
    int spawn_grace_ticks = 0;
    int anim_tick = 0;
    EntityId homing_target_id = 0;
};

class TriggerSystem;

class ProjectileSystem {
public:
    void clear();

    void spawn_bullet(float muzzle_x, float muzzle_y, float aim_x, float aim_y, EntityId shooter_id,
                      int damage = 3, WeaponId source_weapon = WeaponId::Pistol);
    void spawn_shotgun_pellets(float muzzle_x, float muzzle_y, float aim_x, float aim_y,
                               EntityId shooter_id, int pellet_count, int pellet_damage = 3,
                               WeaponId source_weapon = WeaponId::Shotgun1);
    void spawn_flame(float muzzle_x, float muzzle_y, float aim_x, float aim_y,
                     EntityId shooter_id);
    void spawn_rocket(float x, float y, float aim_x, float aim_y, EntityId shooter_id);
    void spawn_plasma(float x, float y, float aim_x, float aim_y, EntityId shooter_id,
                      int damage = 5);
    void spawn_bfg(float x, float y, float aim_x, float aim_y, EntityId shooter_id);
    void spawn_monster_attack(map::MonsterType type, float muzzle_x, float muzzle_y, float aim_x,
                              float aim_y, EntityId shooter_id);

    void tick(const MapCollision& collision, TriggerSystem* triggers, PlayerState& player,
              std::vector<ShootableTarget>& targets, EventBus* events, int map_width,
              int map_height);

    [[nodiscard]] const std::vector<Projectile>& projectiles() const { return projectiles_; }

private:
    std::size_t allocate_slot();
    void launch_projectile(std::size_t index, float x, float y, float aim_x, float aim_y,
                           int speed, int lifetime_ticks);
    void spawn_fireball(ProjectileKind kind, float muzzle_x, float muzzle_y, float aim_x,
                        float aim_y, EntityId shooter_id, int damage, float size, int speed,
                        int lifetime);
    void spawn_skelfire(float muzzle_x, float muzzle_y, float aim_x, float aim_y,
                        EntityId shooter_id, EntityId homing_target_id);
    void destroy_projectile(std::size_t index);
    void explode_rocket(std::size_t index, TriggerSystem* triggers, PlayerState& player,
                        std::vector<ShootableTarget>& targets, EventBus* events);
    void explode_bfg(std::size_t index, TriggerSystem* triggers, PlayerState& player,
                     std::vector<ShootableTarget>& targets, EventBus* events);
    void tick_projectile(std::size_t index, const MapCollision& collision, TriggerSystem* triggers,
                         PlayerState& player, std::vector<ShootableTarget>& targets,
                         EventBus* events, int map_width, int map_height);
    bool tick_ballistic(std::size_t index, const MapCollision& collision, TriggerSystem* triggers,
                        PlayerState& player, std::vector<ShootableTarget>& targets,
                        EventBus* events, std::uint16_t move_state, bool can_collide);

    std::vector<Projectile> projectiles_;
};

} // namespace d2df::sim
