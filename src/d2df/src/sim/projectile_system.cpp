#include <d2df/sim/projectile_system.hpp>

#include <algorithm>
#include <cmath>

#include <d2df/core/event_bus.hpp>
#include <d2df/core/game_events.hpp>
#include <d2df/sim/combat_common.hpp>
#include <d2df/sim/trigger_system.hpp>

namespace d2df::sim {
namespace {

constexpr float kRocketWidth = 14.0f;
constexpr float kRocketHeight = 14.0f;
constexpr float kPlasmaWidth = 16.0f;
constexpr float kPlasmaHeight = 16.0f;
constexpr int kRocketSpeed = 12;
constexpr int kPlasmaSpeed = 16;
constexpr int kRocketLifetime = 900;
constexpr int kPlasmaLifetime = 550;
constexpr int kRocketExplosionRadius = 60;
constexpr int kRocketExplosionDamage = 80;
constexpr int kPlasmaDamage = 5;
constexpr int kRocketDirectDamage = 10;
constexpr float kBfgWidth = 32.0f;
constexpr float kBfgHeight = 32.0f;
constexpr int kBfgSpeed = 16;
constexpr int kBfgLifetime = 700;
constexpr int kBfgExplosionRadius = 128;
constexpr int kBfgSplashDamage = 50;
constexpr int kBfgDirectDamage = 100;
constexpr int kSpawnGraceTicks = 4;
constexpr float kSpawnNudge = 8.0f;

int abs_int(int value) {
    return value < 0 ? -value : value;
}

void set_velocity_toward(Projectile& projectile, float x, float y, float aim_x, float aim_y,
                         int speed) {
    const float dx = aim_x - x;
    const float dy = aim_y - y;
    const int adx = abs_int(static_cast<int>(std::round(dx)));
    const int ady = abs_int(static_cast<int>(std::round(dy)));
    int denom = std::max(adx, ady);
    if (denom == 0) {
        denom = 1;
    }
    projectile.vel_x = static_cast<int>(dx * speed) / denom;
    projectile.vel_y = static_cast<int>(dy * speed) / denom;
}

void compute_spawn_pose(float muzzle_x, float muzzle_y, float aim_x, float aim_y, float width,
                        float height, float& spawn_x, float& spawn_y) {
    const float dx = aim_x - muzzle_x;
    const float dy = aim_y - muzzle_y;
    const float len = std::sqrt(dx * dx + dy * dy);

    const bool firing_right = dx >= 0.0f;
    spawn_x = firing_right ? muzzle_x - width : muzzle_x;
    spawn_y = muzzle_y - height * 0.5f;

    if (len > 1.0f) {
        const float nx = dx / len;
        const float ny = dy / len;
        spawn_x += nx * kSpawnNudge;
        spawn_y += ny * kSpawnNudge;
    }
}

bool overlaps_any_target(const Projectile& projectile, const std::vector<ShootableTarget>& targets,
                         std::size_t& target_index) {
    for (std::size_t i = 0; i < targets.size(); ++i) {
        if (!targets[i].alive()) {
            continue;
        }
        if (rects_overlap(projectile.x, projectile.y, projectile.width, projectile.height,
                          targets[i].x, targets[i].y, targets[i].width, targets[i].height)) {
            target_index = i;
            return true;
        }
    }
    return false;
}

} // namespace

void ProjectileSystem::clear() {
    projectiles_.clear();
}

std::size_t ProjectileSystem::allocate_slot() {
    for (std::size_t i = 0; i < projectiles_.size(); ++i) {
        if (!projectiles_[i].active) {
            return i;
        }
    }
    projectiles_.push_back({});
    return projectiles_.size() - 1;
}

void ProjectileSystem::launch_projectile(std::size_t index, float x, float y, float aim_x,
                                         float aim_y, int speed, int lifetime_ticks) {
    auto& projectile = projectiles_[index];
    projectile.prev_x = x;
    projectile.prev_y = y;
    projectile.x = x;
    projectile.y = y;
    projectile.timeout = lifetime_ticks;
    projectile.spawn_grace_ticks = kSpawnGraceTicks;
    set_velocity_toward(projectile, x, y, aim_x, aim_y, speed);
}

void ProjectileSystem::spawn_rocket(float muzzle_x, float muzzle_y, float aim_x, float aim_y,
                                    EntityId shooter_id) {
    const std::size_t index = allocate_slot();
    auto& projectile = projectiles_[index];
    projectile.active = true;
    projectile.weapon = WeaponId::RocketLauncher;
    projectile.shooter_id = shooter_id;
    projectile.width = kRocketWidth;
    projectile.height = kRocketHeight;

    float spawn_x = 0.0f;
    float spawn_y = 0.0f;
    compute_spawn_pose(muzzle_x, muzzle_y, aim_x, aim_y, kRocketWidth, kRocketHeight, spawn_x,
                       spawn_y);
    launch_projectile(index, spawn_x, spawn_y, aim_x, aim_y, kRocketSpeed, kRocketLifetime);
}

void ProjectileSystem::spawn_plasma(float muzzle_x, float muzzle_y, float aim_x, float aim_y,
                                    EntityId shooter_id) {
    const std::size_t index = allocate_slot();
    auto& projectile = projectiles_[index];
    projectile.active = true;
    projectile.weapon = WeaponId::Plasma;
    projectile.shooter_id = shooter_id;
    projectile.width = kPlasmaWidth;
    projectile.height = kPlasmaHeight;

    float spawn_x = 0.0f;
    float spawn_y = 0.0f;
    compute_spawn_pose(muzzle_x, muzzle_y, aim_x, aim_y, kPlasmaWidth, kPlasmaHeight, spawn_x,
                       spawn_y);
    launch_projectile(index, spawn_x, spawn_y, aim_x, aim_y, kPlasmaSpeed, kPlasmaLifetime);
}

void ProjectileSystem::spawn_bfg(float muzzle_x, float muzzle_y, float aim_x, float aim_y,
                                 EntityId shooter_id) {
    const std::size_t index = allocate_slot();
    auto& projectile = projectiles_[index];
    projectile.active = true;
    projectile.weapon = WeaponId::Bfg;
    projectile.shooter_id = shooter_id;
    projectile.width = kBfgWidth;
    projectile.height = kBfgHeight;

    float spawn_x = 0.0f;
    float spawn_y = 0.0f;
    compute_spawn_pose(muzzle_x, muzzle_y, aim_x, aim_y, kBfgWidth, kBfgHeight, spawn_x, spawn_y);
    launch_projectile(index, spawn_x, spawn_y, aim_x, aim_y, kBfgSpeed, kBfgLifetime);
}

void ProjectileSystem::destroy_projectile(std::size_t index) {
    projectiles_[index].active = false;
    projectiles_[index].timeout = 0;
    projectiles_[index].vel_x = 0;
    projectiles_[index].vel_y = 0;
    projectiles_[index].spawn_grace_ticks = 0;
}

void ProjectileSystem::explode_rocket(std::size_t index, TriggerSystem* triggers,
                                      PlayerState& player, std::vector<ShootableTarget>& targets,
                                      EventBus* events) {
    const auto& projectile = projectiles_[index];
    const float cx = projectile.x + projectile.width * 0.5f;
    const float cy = projectile.y + projectile.height * 0.5f;

    if (triggers != nullptr) {
        triggers->press_shot_circle(cx, cy, static_cast<float>(kRocketExplosionRadius), player,
                                    events);
    }

    apply_damage_to_targets(targets, cx, cy, static_cast<float>(kRocketExplosionRadius),
                            kRocketExplosionDamage, projectile.shooter_id, events,
                            events::DamageSource::Projectile);
    destroy_projectile(index);
}

void ProjectileSystem::explode_bfg(std::size_t index, TriggerSystem* triggers, PlayerState& player,
                                   std::vector<ShootableTarget>& targets, EventBus* events) {
    const auto& projectile = projectiles_[index];
    const float cx = projectile.x + projectile.width * 0.5f;
    const float cy = projectile.y + projectile.height * 0.5f;

    if (triggers != nullptr) {
        triggers->press_shot_circle(cx, cy, static_cast<float>(kBfgExplosionRadius), player,
                                    events);
    }

    apply_damage_to_targets(targets, cx, cy, static_cast<float>(kBfgExplosionRadius),
                            kBfgSplashDamage, projectile.shooter_id, events,
                            events::DamageSource::Projectile);
    destroy_projectile(index);
}

void ProjectileSystem::tick_projectile(std::size_t index, const MapCollision& collision,
                                       TriggerSystem* triggers, PlayerState& player,
                                       std::vector<ShootableTarget>& targets, EventBus* events,
                                       int map_width, int map_height) {
    auto& projectile = projectiles_[index];
    if (!projectile.active) {
        return;
    }

    projectile.prev_x = projectile.x;
    projectile.prev_y = projectile.y;

    float next_x = projectile.x;
    float next_y = projectile.y;
    const std::uint16_t move_state =
        collision.move_object(next_x, next_y, projectile.width, projectile.height, projectile.vel_x,
                              projectile.vel_y, false);
    projectile.x = next_x;
    projectile.y = next_y;

    --projectile.timeout;
    if (projectile.spawn_grace_ticks > 0) {
        --projectile.spawn_grace_ticks;
    }
    const bool can_collide = projectile.spawn_grace_ticks <= 0;

    if (triggers != nullptr && can_collide) {
        triggers->press_shot_rect(projectile.x, projectile.y, projectile.width, projectile.height,
                                  player, events);
    }

    const bool out_of_bounds =
        projectile.x < -1000.0f || projectile.y < -1000.0f ||
        projectile.x > static_cast<float>(map_width) + 1000.0f ||
        projectile.y > static_cast<float>(map_height) + 1000.0f;

    if (out_of_bounds) {
        if (projectile.weapon == WeaponId::RocketLauncher) {
            explode_rocket(index, triggers, player, targets, events);
        } else if (projectile.weapon == WeaponId::Bfg) {
            explode_bfg(index, triggers, player, targets, events);
        } else {
            destroy_projectile(index);
        }
        return;
    }

    if (projectile.timeout <= 0) {
        if (projectile.weapon == WeaponId::RocketLauncher) {
            explode_rocket(index, triggers, player, targets, events);
        } else if (projectile.weapon == WeaponId::Bfg) {
            explode_bfg(index, triggers, player, targets, events);
        } else {
            destroy_projectile(index);
        }
        return;
    }

    const bool hit_solid =
        can_collide && (move_state & (MOVE_HITWALL | MOVE_HITCEIL | MOVE_HITLAND)) != 0;

    std::size_t hit_target_index = 0;
    const bool hit_target =
        can_collide && overlaps_any_target(projectile, targets, hit_target_index);

    if (hit_target && projectile.weapon == WeaponId::Plasma) {
        auto& target = targets[hit_target_index];
        const int health_before = target.health;
        target.apply_damage(kPlasmaDamage);
        publish_entity_damage(events, target.id, projectile.shooter_id,
                              health_before - target.health, events::DamageSource::Projectile,
                              target.health);
        destroy_projectile(index);
        return;
    }

    if (projectile.weapon == WeaponId::RocketLauncher) {
        if (hit_solid || hit_target) {
            if (hit_target) {
                auto& target = targets[hit_target_index];
                const int health_before = target.health;
                target.apply_damage(kRocketDirectDamage);
                publish_entity_damage(events, target.id, projectile.shooter_id,
                                      health_before - target.health,
                                      events::DamageSource::Projectile, target.health);
            }
            explode_rocket(index, triggers, player, targets, events);
        }
        return;
    }

    if (projectile.weapon == WeaponId::Bfg) {
        if (hit_solid || hit_target) {
            if (hit_target) {
                auto& target = targets[hit_target_index];
                const int health_before = target.health;
                target.apply_damage(kBfgDirectDamage);
                publish_entity_damage(events, target.id, projectile.shooter_id,
                                      health_before - target.health,
                                      events::DamageSource::Projectile, target.health);
            }
            explode_bfg(index, triggers, player, targets, events);
        }
        return;
    }

    if (hit_solid) {
        destroy_projectile(index);
    }
}

void ProjectileSystem::tick(const MapCollision& collision, TriggerSystem* triggers,
                            PlayerState& player, std::vector<ShootableTarget>& targets,
                            EventBus* events, int map_width, int map_height) {
    for (std::size_t i = 0; i < projectiles_.size(); ++i) {
        if (projectiles_[i].active) {
            tick_projectile(i, collision, triggers, player, targets, events, map_width,
                            map_height);
        }
    }
}

} // namespace d2df::sim
