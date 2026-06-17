#include <d2df/sim/projectile_system.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

#include <d2df/core/event_bus.hpp>
#include <d2df/core/game_events.hpp>
#include <d2df/core/types.hpp>
#include <d2df/sim/combat_common.hpp>
#include <d2df/sim/effect_types.hpp>
#include <d2df/sim/map_collision.hpp>
#include <d2df/sim/trigger_system.hpp>

namespace d2df::sim {
namespace {

constexpr float kRocketWidth = 14.0f;
constexpr float kRocketHeight = 14.0f;
constexpr float kSkelFireWidth = 14.0f;
constexpr float kSkelFireHeight = 14.0f;
constexpr float kPlasmaWidth = 16.0f;
constexpr float kPlasmaHeight = 16.0f;
constexpr float kBulletSize = 6.0f;
constexpr float kPelletSize = 4.0f;
constexpr float kFireballSize = 16.0f;
constexpr float kFlameSize = 12.0f;

constexpr int kBulletSpeed = 18;
constexpr int kPelletSpeed = 16;
constexpr int kPlasmaSpeed = 16;
constexpr int kFireballSpeed = 16;
constexpr int kFlameSpeed = 10;
constexpr int kRocketSpeed = 12;
constexpr int kSkelFireSpeed = 12;

constexpr int kBulletLifetime = 500;
constexpr int kPelletLifetime = 450;
constexpr int kPlasmaLifetime = 550;
constexpr int kFireballLifetime = 450;
constexpr int kFlameLifetime = 45;
constexpr int kRocketLifetime = 900;

constexpr int kRocketExplosionRadius = 60;
constexpr int kRocketExplosionDamage = 80;
constexpr int kRocketDirectDamage = 10;
constexpr float kBfgWidth = 32.0f;
constexpr float kBfgHeight = 32.0f;
constexpr int kBfgSpeed = 16;
constexpr int kBfgLifetime = 700;
constexpr int kBfgExplosionRadius = 128;
constexpr int kBfgSplashDamage = 50;
constexpr int kBfgDirectDamage = 100;
constexpr int kSpawnGraceTicks = 2;
constexpr float kSpawnNudge = 8.0f;

constexpr int kShotgunSpread[][2] = {
    {0, 0}, {4, -3}, {-3, 2}, {2, 5}, {-5, -2}, {6, 1}, {-2, -6}, {3, 4}, {-4, 3}, {1, -5},
    {-1, 6}, {5, -4}, {-6, 0}, {2, -7}, {7, 2}, {-3, -5}, {4, 6}, {-5, 1}, {0, -8}, {6, -3},
};

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
    const float start_cx = projectile.prev_x + projectile.width * 0.5f;
    const float start_cy = projectile.prev_y + projectile.height * 0.5f;
    const float end_cx = projectile.x + projectile.width * 0.5f;
    const float end_cy = projectile.y + projectile.height * 0.5f;
    const bool use_swept = start_cx != end_cx || start_cy != end_cy;

    float best_distance_sq = std::numeric_limits<float>::max();
    bool found = false;

    for (std::size_t i = 0; i < targets.size(); ++i) {
        if (!targets[i].alive()) {
            continue;
        }
        if (targets[i].id == projectile.shooter_id) {
            continue;
        }

        float hit_distance_sq = 0.0f;
        bool hit = rects_overlap(projectile.x, projectile.y, projectile.width, projectile.height,
                                 targets[i].x, targets[i].y, targets[i].width,
                                 targets[i].height);

        if (!hit && use_swept) {
            float hit_x = 0.0f;
            float hit_y = 0.0f;
            hit = segment_intersects_aabb(start_cx, start_cy, end_cx, end_cy, targets[i].x,
                                          targets[i].y, targets[i].width + projectile.width,
                                          targets[i].height + projectile.height, hit_x, hit_y,
                                          hit_distance_sq);
        }

        if (hit && hit_distance_sq < best_distance_sq) {
            best_distance_sq = hit_distance_sq;
            target_index = i;
            found = true;
        }
    }

    return found;
}

void publish_player_projectile_damage(EventBus* events, int amount, int health_remaining) {
    if (events == nullptr || amount <= 0) {
        return;
    }
    events->publish(events::PlayerDamaged{amount, events::DamageSource::Projectile,
                                          health_remaining});
    events->publish(events::EntityDamaged{kPlayerEntityId, 0, amount,
                                          events::DamageSource::Projectile, health_remaining});
    if (health_remaining <= 0) {
        events->publish(events::PlayerDied{events::DamageSource::Projectile});
        events->publish(events::EntityDied{kPlayerEntityId, events::DamageSource::Projectile});
    }
}

bool is_ballistic_kind(ProjectileKind kind) {
    switch (kind) {
    case ProjectileKind::Bullet:
    case ProjectileKind::ShotgunPellet:
    case ProjectileKind::Plasma:
    case ProjectileKind::ImpFire:
    case ProjectileKind::CacoFire:
    case ProjectileKind::BaronFire:
    case ProjectileKind::BspFire:
    case ProjectileKind::MancubFire:
    case ProjectileKind::Flame:
        return true;
    default:
        return false;
    }
}

void publish_projectile_explosion(EventBus* events, ProjectileKind kind, float cx, float cy) {
    if (events == nullptr) {
        return;
    }
    const auto explosion_kind = explosion_kind_for_projectile(kind);
    if (!explosion_kind.has_value()) {
        return;
    }
    events->publish(events::WorldExplosion{*explosion_kind, cx, cy});
}

void publish_rocket_smoke(EventBus* events, const Projectile& projectile, int move_state) {
    if (events == nullptr) {
        return;
    }
    if (projectile.kind != ProjectileKind::Rocket && projectile.kind != ProjectileKind::SkelFire) {
        return;
    }

    const float cx = projectile.x + projectile.width * 0.5f;
    const float cy = projectile.y + projectile.height * 0.5f;
    const int jitter_x = projectile.anim_tick % 9;
    const int jitter_y = (projectile.anim_tick * 3) % 9;
    const float fx = cx + static_cast<float>(jitter_x);
    const float fy = cy + static_cast<float>(jitter_y);

    if ((move_state & MOVE_INWATER) != 0) {
        events->publish(events::WorldBubblePuff{fx, fy});
        return;
    }

    events->publish(events::WorldSmokePuff{fx, fy});
}

bool resolve_homing_aim(EntityId target_id, const PlayerState& player,
                        const std::vector<ShootableTarget>& targets, float& aim_x, float& aim_y) {
    if (target_id == kPlayerEntityId) {
        if (!player.alive()) {
            return false;
        }
        aim_x = player.x + player.width * 0.5f + static_cast<float>(player.vel_x);
        aim_y = player.y + player.height * 0.5f + static_cast<float>(player.vel_y);
        return true;
    }

    for (const auto& target : targets) {
        if (target.id != target_id || !target.alive()) {
            continue;
        }
        aim_x = target.x + target.width * 0.5f + static_cast<float>(target.vel_x);
        aim_y = target.y + target.height * 0.5f + static_cast<float>(target.vel_y);
        return true;
    }
    return false;
}

void retarget_homing_projectile(Projectile& projectile, const PlayerState& player,
                                const std::vector<ShootableTarget>& targets, int speed) {
    float aim_x = 0.0f;
    float aim_y = 0.0f;
    if (!resolve_homing_aim(projectile.homing_target_id, player, targets, aim_x, aim_y)) {
        return;
    }
    set_velocity_toward(projectile, projectile.x, projectile.y, aim_x, aim_y, speed);
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

void ProjectileSystem::spawn_bullet(float muzzle_x, float muzzle_y, float aim_x, float aim_y,
                                    EntityId shooter_id, int damage, WeaponId source_weapon) {
    const std::size_t index = allocate_slot();
    auto& projectile = projectiles_[index];
    projectile = {};
    projectile.active = true;
    projectile.kind = ProjectileKind::Bullet;
    projectile.weapon = source_weapon;
    projectile.shooter_id = shooter_id;
    projectile.damage = damage;
    projectile.width = kBulletSize;
    projectile.height = kBulletSize;

    float spawn_x = 0.0f;
    float spawn_y = 0.0f;
    compute_spawn_pose(muzzle_x, muzzle_y, aim_x, aim_y, kBulletSize, kBulletSize, spawn_x,
                       spawn_y);
    launch_projectile(index, spawn_x, spawn_y, aim_x, aim_y, kBulletSpeed, kBulletLifetime);
}

void ProjectileSystem::spawn_shotgun_pellets(float muzzle_x, float muzzle_y, float aim_x,
                                             float aim_y, EntityId shooter_id, int pellet_count,
                                             int pellet_damage, WeaponId source_weapon) {
    const int count = std::min(pellet_count, static_cast<int>(sizeof(kShotgunSpread) /
                                                              sizeof(kShotgunSpread[0])));
    for (int i = 0; i < count; ++i) {
        const float ox = muzzle_x + static_cast<float>(kShotgunSpread[i][0]);
        const float oy = muzzle_y + static_cast<float>(kShotgunSpread[i][1]);
        const float ax = aim_x + static_cast<float>(kShotgunSpread[i][0]);
        const float ay = aim_y + static_cast<float>(kShotgunSpread[i][1]);

        const std::size_t index = allocate_slot();
        auto& projectile = projectiles_[index];
        projectile = {};
        projectile.active = true;
        projectile.kind = ProjectileKind::ShotgunPellet;
        projectile.weapon = source_weapon;
        projectile.shooter_id = shooter_id;
        projectile.damage = pellet_damage;
        projectile.width = kPelletSize;
        projectile.height = kPelletSize;

        float spawn_x = 0.0f;
        float spawn_y = 0.0f;
        compute_spawn_pose(ox, oy, ax, ay, kPelletSize, kPelletSize, spawn_x, spawn_y);
        launch_projectile(index, spawn_x, spawn_y, ax, ay, kPelletSpeed, kPelletLifetime);
    }
}

void ProjectileSystem::spawn_flame(float muzzle_x, float muzzle_y, float aim_x, float aim_y,
                                   EntityId shooter_id) {
    spawn_fireball(ProjectileKind::Flame, muzzle_x, muzzle_y, aim_x, aim_y, shooter_id, 3,
                   kFlameSize, kFlameSpeed, kFlameLifetime);
}

void ProjectileSystem::spawn_fireball(ProjectileKind kind, float muzzle_x, float muzzle_y,
                                      float aim_x, float aim_y, EntityId shooter_id, int damage,
                                      float size, int speed, int lifetime) {
    const std::size_t index = allocate_slot();
    auto& projectile = projectiles_[index];
    projectile = {};
    projectile.active = true;
    projectile.kind = kind;
    projectile.weapon = WeaponId::Plasma;
    projectile.shooter_id = shooter_id;
    projectile.damage = damage;
    projectile.width = size;
    projectile.height = size;

    float spawn_x = 0.0f;
    float spawn_y = 0.0f;
    compute_spawn_pose(muzzle_x, muzzle_y, aim_x, aim_y, size, size, spawn_x, spawn_y);
    launch_projectile(index, spawn_x, spawn_y, aim_x, aim_y, speed, lifetime);
}

void ProjectileSystem::spawn_rocket(float muzzle_x, float muzzle_y, float aim_x, float aim_y,
                                    EntityId shooter_id) {
    const std::size_t index = allocate_slot();
    auto& projectile = projectiles_[index];
    projectile = {};
    projectile.active = true;
    projectile.kind = ProjectileKind::Rocket;
    projectile.weapon = WeaponId::RocketLauncher;
    projectile.shooter_id = shooter_id;
    projectile.damage = kRocketDirectDamage;
    projectile.width = kRocketWidth;
    projectile.height = kRocketHeight;

    float spawn_x = 0.0f;
    float spawn_y = 0.0f;
    compute_spawn_pose(muzzle_x, muzzle_y, aim_x, aim_y, kRocketWidth, kRocketHeight, spawn_x,
                       spawn_y);
    launch_projectile(index, spawn_x, spawn_y, aim_x, aim_y, kRocketSpeed, kRocketLifetime);
}

void ProjectileSystem::spawn_skelfire(float muzzle_x, float muzzle_y, float aim_x, float aim_y,
                                      EntityId shooter_id, EntityId homing_target_id) {
    const std::size_t index = allocate_slot();
    auto& projectile = projectiles_[index];
    projectile = {};
    projectile.active = true;
    projectile.kind = ProjectileKind::SkelFire;
    projectile.weapon = WeaponId::RocketLauncher;
    projectile.shooter_id = shooter_id;
    projectile.homing_target_id = homing_target_id;
    projectile.damage = kRocketDirectDamage;
    projectile.width = kSkelFireWidth;
    projectile.height = kSkelFireHeight;

    float spawn_x = 0.0f;
    float spawn_y = 0.0f;
    compute_spawn_pose(muzzle_x, muzzle_y, aim_x, aim_y, kSkelFireWidth, kSkelFireHeight, spawn_x,
                       spawn_y);
    launch_projectile(index, spawn_x, spawn_y, aim_x, aim_y, kSkelFireSpeed, kRocketLifetime);
}

void ProjectileSystem::spawn_plasma(float muzzle_x, float muzzle_y, float aim_x, float aim_y,
                                      EntityId shooter_id, int damage) {
    const std::size_t index = allocate_slot();
    auto& projectile = projectiles_[index];
    projectile = {};
    projectile.active = true;
    projectile.kind = ProjectileKind::Plasma;
    projectile.weapon = WeaponId::Plasma;
    projectile.shooter_id = shooter_id;
    projectile.damage = damage;
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
    projectile = {};
    projectile.active = true;
    projectile.kind = ProjectileKind::Bfg;
    projectile.weapon = WeaponId::Bfg;
    projectile.shooter_id = shooter_id;
    projectile.damage = kBfgDirectDamage;
    projectile.width = kBfgWidth;
    projectile.height = kBfgHeight;

    float spawn_x = 0.0f;
    float spawn_y = 0.0f;
    compute_spawn_pose(muzzle_x, muzzle_y, aim_x, aim_y, kBfgWidth, kBfgHeight, spawn_x, spawn_y);
    launch_projectile(index, spawn_x, spawn_y, aim_x, aim_y, kBfgSpeed, kBfgLifetime);
}

void ProjectileSystem::spawn_monster_attack(map::MonsterType type, float muzzle_x, float muzzle_y,
                                            float aim_x, float aim_y, EntityId shooter_id) {
    switch (type) {
    case map::MonsterType::Zomby:
        spawn_bullet(muzzle_x, muzzle_y, aim_x, aim_y, shooter_id, 3, WeaponId::Pistol);
        break;
    case map::MonsterType::Serg:
        spawn_shotgun_pellets(muzzle_x, muzzle_y, aim_x, aim_y, shooter_id, 10, 3,
                              WeaponId::Shotgun1);
        break;
    case map::MonsterType::Man:
        spawn_shotgun_pellets(muzzle_x, muzzle_y, aim_x, aim_y, shooter_id, 20, 3,
                              WeaponId::Shotgun2);
        break;
    case map::MonsterType::Cgun:
    case map::MonsterType::Spider:
        spawn_bullet(muzzle_x, muzzle_y, aim_x, aim_y, shooter_id, 3, WeaponId::Chaingun);
        break;
    case map::MonsterType::Imp:
        spawn_fireball(ProjectileKind::ImpFire, muzzle_x, muzzle_y, aim_x, aim_y, shooter_id, 4,
                       kFireballSize, kFireballSpeed, kFireballLifetime);
        break;
    case map::MonsterType::Caco:
        spawn_fireball(ProjectileKind::CacoFire, muzzle_x, muzzle_y, aim_x, aim_y, shooter_id, 4,
                       kFireballSize, kFireballSpeed, kFireballLifetime);
        break;
    case map::MonsterType::Baron:
    case map::MonsterType::Knight:
        spawn_fireball(ProjectileKind::BaronFire, muzzle_x, muzzle_y, aim_x, aim_y, shooter_id, 5,
                       kFireballSize, kFireballSpeed, kFireballLifetime);
        break;
    case map::MonsterType::Cyber:
        spawn_rocket(muzzle_x, muzzle_y, aim_x, aim_y, shooter_id);
        break;
    case map::MonsterType::Skel:
        spawn_skelfire(muzzle_x, muzzle_y, aim_x, aim_y, shooter_id, kPlayerEntityId);
        break;
    case map::MonsterType::Bsp:
        spawn_fireball(ProjectileKind::BspFire, muzzle_x, muzzle_y, aim_x, aim_y, shooter_id, 10,
                       kFireballSize, kFireballSpeed, kFireballLifetime);
        break;
    case map::MonsterType::Robo:
        spawn_plasma(muzzle_x, muzzle_y, aim_x, aim_y, shooter_id, 4);
        break;
    case map::MonsterType::Mancub:
        spawn_fireball(ProjectileKind::MancubFire, muzzle_x, muzzle_y, aim_x, aim_y, shooter_id, 3,
                       32.0f, kFireballSpeed, kFireballLifetime);
        break;
    default:
        spawn_bullet(muzzle_x, muzzle_y, aim_x, aim_y, shooter_id, 3, WeaponId::Pistol);
        break;
    }
}

void ProjectileSystem::destroy_projectile(std::size_t index) {
    projectiles_[index].active = false;
    projectiles_[index].timeout = 0;
    projectiles_[index].vel_x = 0;
    projectiles_[index].vel_y = 0;
    projectiles_[index].spawn_grace_ticks = 0;
    projectiles_[index].anim_tick = 0;
}

void ProjectileSystem::explode_rocket(std::size_t index, TriggerSystem* triggers,
                                      PlayerState& player, std::vector<ShootableTarget>& targets,
                                      EventBus* events) {
    const auto& projectile = projectiles_[index];
    const float cx = projectile.x + projectile.width * 0.5f;
    const float cy = projectile.y + projectile.height * 0.5f;

    apply_area_explosion(cx, cy, static_cast<float>(kRocketExplosionRadius), kRocketExplosionDamage,
                         player, targets, projectile.shooter_id, events,
                         events::DamageSource::Projectile, triggers);

    if (events != nullptr) {
        const auto explosion_kind = projectile.kind == ProjectileKind::SkelFire
                                        ? events::ExplosionKind::SkelFire
                                        : events::ExplosionKind::Rocket;
        events->publish(events::WorldExplosion{explosion_kind, cx, cy});
    }
    destroy_projectile(index);
}

void ProjectileSystem::explode_bfg(std::size_t index, TriggerSystem* triggers, PlayerState& player,
                                   std::vector<ShootableTarget>& targets, EventBus* events) {
    const auto& projectile = projectiles_[index];
    const float cx = projectile.x + projectile.width * 0.5f;
    const float cy = projectile.y + projectile.height * 0.5f;

    apply_area_explosion(cx, cy, static_cast<float>(kBfgExplosionRadius), kBfgSplashDamage, player,
                         targets, projectile.shooter_id, events, events::DamageSource::Projectile,
                         triggers);

    if (events != nullptr) {
        events->publish(events::WorldExplosion{events::ExplosionKind::Bfg, cx, cy});
    }
    destroy_projectile(index);
}

bool ProjectileSystem::tick_ballistic(std::size_t index, const MapCollision& /*collision*/,
                                      TriggerSystem* /*triggers*/, PlayerState& player,
                                      std::vector<ShootableTarget>& targets, EventBus* events,
                                      std::uint16_t move_state, bool can_hit_world,
                                      bool hit_target, std::size_t hit_target_index) {
    auto& projectile = projectiles_[index];
    const bool from_player = projectile.shooter_id == kPlayerEntityId;
    const float cx = projectile.x + projectile.width * 0.5f;
    const float cy = projectile.y + projectile.height * 0.5f;

    if (can_hit_world && !from_player && player.alive() &&
        rects_overlap(projectile.x, projectile.y, projectile.width, projectile.height, player.x,
                      player.y, player.width, player.height)) {
        const int health_before = player.health();
        player.apply_damage(projectile.damage);
        publish_player_projectile_damage(events, health_before - player.health(), player.health());
        publish_projectile_explosion(events, projectile.kind, cx, cy);
        destroy_projectile(index);
        return true;
    }

    if (hit_target && from_player) {
        auto& target = targets[hit_target_index];
        const int health_before = target.health;
        target.apply_damage(projectile.damage, projectile.shooter_id);
        publish_entity_damage(events, target.id, projectile.shooter_id,
                              health_before - target.health, events::DamageSource::Projectile,
                              target.health);
        publish_projectile_explosion(events, projectile.kind, cx, cy);
        destroy_projectile(index);
        return true;
    }

    if (can_hit_world && (move_state & (MOVE_HITWALL | MOVE_HITCEIL | MOVE_HITLAND)) != 0) {
        publish_projectile_explosion(events, projectile.kind, cx, cy);
        destroy_projectile(index);
        return true;
    }

    return false;
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
    ++projectile.anim_tick;

    float next_x = projectile.x;
    float next_y = projectile.y;
    const std::uint16_t move_state =
        collision.move_object(next_x, next_y, projectile.width, projectile.height, projectile.vel_x,
                              projectile.vel_y, false);
    projectile.x = next_x;
    projectile.y = next_y;

    if (projectile.kind == ProjectileKind::Rocket || projectile.kind == ProjectileKind::SkelFire) {
        publish_rocket_smoke(events, projectile, move_state);
    }

    --projectile.timeout;
    if (projectile.spawn_grace_ticks > 0) {
        --projectile.spawn_grace_ticks;
    }
    // Grace only suppresses world/player collisions so muzzle-inside-body shots do not self-hit.
    // Targets are always checked so point-blank shots cannot tunnel through during grace.
    const bool can_hit_world = projectile.spawn_grace_ticks <= 0;

    if (triggers != nullptr && can_hit_world) {
        const float cx0 = projectile.prev_x + projectile.width * 0.5f;
        const float cy0 = projectile.prev_y + projectile.height * 0.5f;
        const float cx1 = projectile.x + projectile.width * 0.5f;
        const float cy1 = projectile.y + projectile.height * 0.5f;
        triggers->press_shot_line(cx0, cy0, cx1, cy1, player, events);
    }

    const bool out_of_bounds =
        projectile.x < -1000.0f || projectile.y < -1000.0f ||
        projectile.x > static_cast<float>(map_width) + 1000.0f ||
        projectile.y > static_cast<float>(map_height) + 1000.0f;

    if (out_of_bounds) {
        if (projectile.kind == ProjectileKind::Rocket ||
            projectile.kind == ProjectileKind::SkelFire) {
            explode_rocket(index, triggers, player, targets, events);
        } else if (projectile.kind == ProjectileKind::Bfg) {
            explode_bfg(index, triggers, player, targets, events);
        } else {
            destroy_projectile(index);
        }
        return;
    }

    if (projectile.timeout <= 0) {
        if (projectile.kind == ProjectileKind::Rocket ||
            projectile.kind == ProjectileKind::SkelFire) {
            explode_rocket(index, triggers, player, targets, events);
        } else if (projectile.kind == ProjectileKind::Bfg) {
            explode_bfg(index, triggers, player, targets, events);
        } else {
            destroy_projectile(index);
        }
        return;
    }

    const bool hit_solid =
        can_hit_world && (move_state & (MOVE_HITWALL | MOVE_HITCEIL | MOVE_HITLAND)) != 0;

    std::size_t hit_target_index = 0;
    const bool hit_target = overlaps_any_target(projectile, targets, hit_target_index);

    if (is_ballistic_kind(projectile.kind)) {
        if (tick_ballistic(index, collision, triggers, player, targets, events, move_state,
                           can_hit_world, hit_target, hit_target_index)) {
            return;
        }
    }

    if (projectile.kind == ProjectileKind::Rocket || projectile.kind == ProjectileKind::SkelFire) {
        const bool from_player = projectile.shooter_id == kPlayerEntityId;
        const bool hit_player =
            can_hit_world && !from_player && player.alive() &&
            rects_overlap(projectile.x, projectile.y, projectile.width, projectile.height, player.x,
                          player.y, player.width, player.height);

        if (hit_solid || hit_target || hit_player) {
            if (hit_target) {
                auto& target = targets[hit_target_index];
                const int health_before = target.health;
                target.apply_damage(kRocketDirectDamage, projectile.shooter_id);
                publish_entity_damage(events, target.id, projectile.shooter_id,
                                      health_before - target.health,
                                      events::DamageSource::Projectile, target.health);
            }
            if (hit_player) {
                const int health_before = player.health();
                player.apply_damage(kRocketDirectDamage);
                publish_player_projectile_damage(events, health_before - player.health(),
                                                 player.health());
            }
            explode_rocket(index, triggers, player, targets, events);
            return;
        }

        if (projectile.kind == ProjectileKind::SkelFire && can_hit_world &&
            projectile.homing_target_id != 0) {
            retarget_homing_projectile(projectile, player, targets, kSkelFireSpeed);
        }
        return;
    }

    if (projectile.kind == ProjectileKind::Bfg) {
        if (hit_solid || hit_target) {
            if (hit_target) {
                auto& target = targets[hit_target_index];
                const int health_before = target.health;
                target.apply_damage(kBfgDirectDamage, projectile.shooter_id);
                publish_entity_damage(events, target.id, projectile.shooter_id,
                                      health_before - target.health,
                                      events::DamageSource::Projectile, target.health);
            }
            explode_bfg(index, triggers, player, targets, events);
        }
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
