#include <d2df/sim/weapon_system.hpp>

#include <algorithm>
#include <cmath>

#include <d2df/core/event_bus.hpp>
#include <d2df/core/game_events.hpp>
#include <d2df/sim/combat_common.hpp>
#include <d2df/sim/projectile_system.hpp>
#include <d2df/sim/trigger_system.hpp>

namespace d2df::sim {
namespace {

constexpr float kWeaponPointLeftX = 16.0f;
constexpr float kWeaponPointRightX = 47.0f;
constexpr float kWeaponPointY = 32.0f;

// Deterministic shotgun spread offsets (legacy uses random ±8).
constexpr int kShotgunSpread[][2] = {
    {0, 0}, {4, -3}, {-3, 2}, {2, 5}, {-5, -2}, {6, 1}, {-2, -6}, {3, 4}, {-4, 3}, {1, -5},
    {-1, 6}, {5, -4}, {-6, 0}, {2, -7}, {7, 2}, {-3, -5}, {4, 6}, {-5, 1}, {0, -8}, {6, -3},
};

void fire_melee_box(std::vector<ShootableTarget>& targets, EntityId shooter_id, EventBus* events,
                    float box_x, float box_y, float box_width, float box_height, int damage,
                    events::DamageSource source) {
    for (auto& target : targets) {
        if (!target.alive()) {
            continue;
        }
        if (!rects_overlap(box_x, box_y, box_width, box_height, target.x, target.y, target.width,
                           target.height)) {
            continue;
        }
        const int health_before = target.health;
        target.apply_damage(damage, shooter_id);
        publish_entity_damage(events, target.id, shooter_id, health_before - target.health, source,
                              target.health);
        return;
    }
}

void compute_muzzle(const PlayerState& player, const PlayerCombat& combat, float& wx, float& wy,
                    float& xd, float& yd, int aim_vertical) {
    const float weapon_x = combat.facing_left ? kWeaponPointLeftX : kWeaponPointRightX;
    wx = player.x + weapon_x;
    wy = player.y + kWeaponPointY;
    xd = wx + (combat.facing_left ? -30.0f : 30.0f);
    yd = wy + static_cast<float>(aim_vertical);
}

float aim_vertical_offset(const PlayerInput& input) {
    if (input.aim_up) {
        return -42.0f;
    }
    if (input.aim_down) {
        return 19.0f;
    }
    return 0.0f;
}

} // namespace

void WeaponSystem::compute_ray_end(float wx, float wy, float xd, float yd, int map_width,
                                   float& ray_x2, float& ray_y2) const {
    const float angle_rad = std::atan2(yd - wy, xd - wx);
    const float extent = static_cast<float>(map_width > 0 ? map_width : 4096);
    ray_x2 = wx + std::cos(angle_rad) * extent;
    ray_y2 = wy + std::sin(angle_rad) * extent;
}

void WeaponSystem::publish_player_fired(EventBus* events, EntityId shooter_id, WeaponId weapon,
                                        float ox, float oy, float ax, float ay) {
    if (events == nullptr) {
        return;
    }
    events->publish(events::PlayerFired{shooter_id, static_cast<std::uint8_t>(weapon), ox, oy, ax,
                                        ay});
}

void WeaponSystem::fire_hitscan(PlayerState& player, const MapCollision& collision,
                                std::vector<ShootableTarget>& targets, TriggerSystem* triggers,
                                EntityId shooter_id, EventBus* events, float x0, float y0,
                                float x1, float y1, int damage, WeaponId /*weapon*/,
                                bool check_shot_trigger) {
    if (x1 == x0 && y1 == y0) {
        return;
    }

    const auto wall_hit = collision.trace_solid_ray(x0, y0, x1, y1);
    const float wall_limit_sq =
        wall_hit.hit ? wall_hit.distance_sq : (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);

    float hit_x = x1;
    float hit_y = y1;
    if (wall_hit.hit) {
        hit_x = wall_hit.x;
        hit_y = wall_hit.y;
    }

    if (check_shot_trigger && triggers != nullptr) {
        triggers->press_shot_line(x0, y0, hit_x, hit_y, player, events);
    }

    damage_targets_along_ray(targets, x0, y0, x1, y1, wall_limit_sq, damage, shooter_id, events,
                             events::DamageSource::Weapon);
}

void WeaponSystem::fire_shotgun(PlayerState& player, const MapCollision& collision,
                                std::vector<ShootableTarget>& targets, TriggerSystem* triggers,
                                EntityId shooter_id, EventBus* events, float wx, float wy,
                                float ray_x2, float ray_y2, int pellet_count, WeaponId weapon) {
    const int count = std::min(pellet_count, static_cast<int>(sizeof(kShotgunSpread) /
                                                              sizeof(kShotgunSpread[0])));
    for (int i = 0; i < count; ++i) {
        const float ox = wx + static_cast<float>(kShotgunSpread[i][0]);
        const float oy = wy + static_cast<float>(kShotgunSpread[i][1]);
        const float ax = ray_x2 + static_cast<float>(kShotgunSpread[i][0]);
        const float ay = ray_y2 + static_cast<float>(kShotgunSpread[i][1]);
        fire_hitscan(player, collision, targets, triggers, shooter_id, events, ox, oy, ax, ay, 3,
                     weapon, i == 0);
    }
}

void WeaponSystem::fire_melee(PlayerState& player, std::vector<ShootableTarget>& targets,
                              EntityId shooter_id, EventBus* events, float box_width,
                              float box_height, int damage, events::DamageSource source) {
    for (auto& target : targets) {
        if (!target.alive()) {
            continue;
        }
        if (!rects_overlap(player.x, player.y, box_width, box_height, target.x, target.y,
                           target.width, target.height)) {
            continue;
        }
        const int health_before = target.health;
        target.apply_damage(damage, shooter_id);
        publish_entity_damage(events, target.id, shooter_id, health_before - target.health, source,
                              target.health);
        return;
    }
}

void WeaponSystem::try_fire(PlayerState& player, const MapCollision& collision,
                            std::vector<ShootableTarget>& targets, TriggerSystem* triggers,
                            ProjectileSystem* projectiles, EntityId shooter_id, EventBus* events,
                            int map_width, const PlayerInput& input) {
    auto& combat = player.combat();
    const WeaponId weapon = combat.current_weapon;

    if (combat.is_reloading(weapon)) {
        return;
    }

    if (combat.bfg_charge_ticks >= 0 && weapon != WeaponId::Bfg) {
        return;
    }

    if (!combat.has_ammo_for_weapon(weapon)) {
        return;
    }

    float wx = 0.0f;
    float wy = 0.0f;
    float xd = 0.0f;
    float yd = 0.0f;
    const int aim_offset = static_cast<int>(aim_vertical_offset(input));
    compute_muzzle(player, combat, wx, wy, xd, yd, aim_offset);

    float ray_x2 = 0.0f;
    float ray_y2 = 0.0f;
    compute_ray_end(wx, wy, xd, yd, map_width, ray_x2, ray_y2);

    switch (weapon) {
    case WeaponId::Knuckles:
        publish_player_fired(events, shooter_id, WeaponId::Knuckles, wx, wy, xd, yd);
        fire_melee(player, targets, shooter_id, events, 39.0f, 52.0f, 3,
                   events::DamageSource::Melee);
        combat.reloading[static_cast<std::size_t>(WeaponId::Knuckles)] =
            weapon_reload_ticks(WeaponId::Knuckles);
        break;

    case WeaponId::Saw:
        publish_player_fired(events, shooter_id, WeaponId::Saw, wx, wy, xd, yd);
        fire_melee(player, targets, shooter_id, events, 32.0f, 52.0f, 3,
                   events::DamageSource::Melee);
        combat.reloading[static_cast<std::size_t>(WeaponId::Saw)] =
            weapon_reload_ticks(WeaponId::Saw);
        break;

    case WeaponId::Pistol: {
        const auto ammo_type = weapon_ammo_type(WeaponId::Pistol);
        combat.ammo[static_cast<std::size_t>(ammo_type)] -= 1;
        combat.reloading[static_cast<std::size_t>(WeaponId::Pistol)] =
            weapon_reload_ticks(WeaponId::Pistol);
        publish_player_fired(events, shooter_id, WeaponId::Pistol, wx, wy, ray_x2, ray_y2);
        fire_hitscan(player, collision, targets, triggers, shooter_id, events, wx, wy, ray_x2,
                     ray_y2, 3, WeaponId::Pistol, true);
        break;
    }

    case WeaponId::Shotgun1: {
        const auto ammo_type = weapon_ammo_type(WeaponId::Shotgun1);
        combat.ammo[static_cast<std::size_t>(ammo_type)] -= 1;
        combat.reloading[static_cast<std::size_t>(WeaponId::Shotgun1)] =
            weapon_reload_ticks(WeaponId::Shotgun1);
        publish_player_fired(events, shooter_id, WeaponId::Shotgun1, wx, wy, ray_x2, ray_y2);
        fire_shotgun(player, collision, targets, triggers, shooter_id, events, wx, wy, ray_x2,
                     ray_y2, 10, WeaponId::Shotgun1);
        break;
    }

    case WeaponId::Shotgun2: {
        const auto ammo_type = weapon_ammo_type(WeaponId::Shotgun2);
        combat.ammo[static_cast<std::size_t>(ammo_type)] -= 2;
        combat.reloading[static_cast<std::size_t>(WeaponId::Shotgun2)] =
            weapon_reload_ticks(WeaponId::Shotgun2);
        publish_player_fired(events, shooter_id, WeaponId::Shotgun2, wx, wy, ray_x2, ray_y2);
        fire_shotgun(player, collision, targets, triggers, shooter_id, events, wx, wy, ray_x2,
                     ray_y2, 20, WeaponId::Shotgun2);
        break;
    }

    case WeaponId::Chaingun: {
        const auto ammo_type = weapon_ammo_type(WeaponId::Chaingun);
        combat.ammo[static_cast<std::size_t>(ammo_type)] -= 1;
        combat.reloading[static_cast<std::size_t>(WeaponId::Chaingun)] =
            weapon_reload_ticks(WeaponId::Chaingun);
        publish_player_fired(events, shooter_id, WeaponId::Chaingun, wx, wy, ray_x2, ray_y2);
        fire_hitscan(player, collision, targets, triggers, shooter_id, events, wx, wy, ray_x2,
                     ray_y2, 3, WeaponId::Chaingun, true);
        break;
    }

    case WeaponId::RocketLauncher: {
        if (projectiles == nullptr) {
            return;
        }
        const auto ammo_type = weapon_ammo_type(WeaponId::RocketLauncher);
        combat.ammo[static_cast<std::size_t>(ammo_type)] -= 1;
        combat.reloading[static_cast<std::size_t>(WeaponId::RocketLauncher)] =
            weapon_reload_ticks(WeaponId::RocketLauncher);
        publish_player_fired(events, shooter_id, WeaponId::RocketLauncher, wx, wy, ray_x2, ray_y2);
        projectiles->spawn_rocket(wx, wy, ray_x2, ray_y2, shooter_id);
        break;
    }

    case WeaponId::Plasma: {
        if (projectiles == nullptr) {
            return;
        }
        const auto ammo_type = weapon_ammo_type(WeaponId::Plasma);
        combat.ammo[static_cast<std::size_t>(ammo_type)] -= 1;
        combat.reloading[static_cast<std::size_t>(WeaponId::Plasma)] =
            weapon_reload_ticks(WeaponId::Plasma);
        publish_player_fired(events, shooter_id, WeaponId::Plasma, wx, wy, ray_x2, ray_y2);
        projectiles->spawn_plasma(wx, wy, ray_x2, ray_y2, shooter_id);
        break;
    }

    case WeaponId::Flamethrower: {
        const auto ammo_type = weapon_ammo_type(WeaponId::Flamethrower);
        combat.ammo[static_cast<std::size_t>(ammo_type)] -= 1;
        combat.reloading[static_cast<std::size_t>(WeaponId::Flamethrower)] =
            weapon_reload_ticks(WeaponId::Flamethrower);
        publish_player_fired(events, shooter_id, WeaponId::Flamethrower, wx, wy, ray_x2, ray_y2);
        const float flame_x =
            combat.facing_left ? player.x - 40.0f : player.x + player.width - 8.0f;
        fire_melee_box(targets, shooter_id, events, flame_x, player.y + 8.0f, 48.0f, 40.0f, 3,
                       events::DamageSource::Weapon);
        break;
    }

    case WeaponId::Bfg: {
        if (projectiles == nullptr || combat.bfg_charge_ticks >= 0) {
            return;
        }
        const auto ammo_type = weapon_ammo_type(WeaponId::Bfg);
        combat.ammo[static_cast<std::size_t>(ammo_type)] -= 40;
        combat.bfg_charge_ticks = 17;
        combat.bfg_muzzle_x = wx;
        combat.bfg_muzzle_y = wy;
        combat.bfg_aim_x = ray_x2;
        combat.bfg_aim_y = ray_y2;
        publish_player_fired(events, shooter_id, WeaponId::Bfg, wx, wy, ray_x2, ray_y2);
        break;
    }

    case WeaponId::SuperChaingun: {
        const auto ammo_type = weapon_ammo_type(WeaponId::SuperChaingun);
        combat.ammo[static_cast<std::size_t>(ammo_type)] -= 1;
        combat.reloading[static_cast<std::size_t>(WeaponId::SuperChaingun)] =
            weapon_reload_ticks(WeaponId::SuperChaingun);
        publish_player_fired(events, shooter_id, WeaponId::SuperChaingun, wx, wy, ray_x2, ray_y2);
        fire_shotgun(player, collision, targets, triggers, shooter_id, events, wx, wy, ray_x2,
                     ray_y2, 10, WeaponId::SuperChaingun);
        break;
    }

    default:
        break;
    }
}

void WeaponSystem::tick_bfg_charge(PlayerState& player, ProjectileSystem* projectiles,
                                   EntityId shooter_id, EventBus* events) {
    auto& combat = player.combat();
    if (combat.bfg_charge_ticks < 0) {
        return;
    }

    if (combat.bfg_charge_ticks > 0) {
        --combat.bfg_charge_ticks;
        return;
    }

    if (projectiles != nullptr) {
        projectiles->spawn_bfg(combat.bfg_muzzle_x, combat.bfg_muzzle_y, combat.bfg_aim_x,
                               combat.bfg_aim_y, shooter_id);
        publish_player_fired(events, shooter_id, WeaponId::Bfg, combat.bfg_muzzle_x,
                             combat.bfg_muzzle_y, combat.bfg_aim_x, combat.bfg_aim_y);
    }

    combat.reloading[static_cast<std::size_t>(WeaponId::Bfg)] =
        weapon_reload_ticks(WeaponId::Bfg);
    combat.bfg_charge_ticks = -1;
}

void WeaponSystem::tick(PlayerState& player, const MapCollision& collision, PlayerInput input,
                        std::vector<ShootableTarget>& targets, TriggerSystem* triggers,
                        ProjectileSystem* projectiles, EntityId shooter_id, EventBus* events,
                        int map_width) {
    auto& combat = player.combat();
    combat.tick_reload();
    tick_bfg_charge(player, projectiles, shooter_id, events);

    if (input.weapon_prev) {
        combat.cycle_weapon(-1);
    }
    if (input.weapon_next) {
        combat.cycle_weapon(1);
    }
    if (input.weapon_select_request >= 0 &&
        input.weapon_select_request <= static_cast<int>(WeaponId::Last)) {
        combat.select_weapon(static_cast<WeaponId>(input.weapon_select_request));
    }

    if (!input.fire) {
        return;
    }

    try_fire(player, collision, targets, triggers, projectiles, shooter_id, events, map_width,
             input);
}

} // namespace d2df::sim
