#include <d2df/sim/monster_system.hpp>

#include <algorithm>
#include <cmath>

#include <d2df/core/event_bus.hpp>
#include <d2df/core/game_events.hpp>
#include <d2df/core/types.hpp>
#include <d2df/map/monster_types.hpp>
#include <d2df/sim/combat_common.hpp>
#include <d2df/sim/projectile_system.hpp>

namespace d2df::sim {
namespace {

constexpr int kMonsterMeleeCooldown = 18;
constexpr int kMonsterMeleeDamage = 6;
constexpr float kMonsterMeleeRangeX = 56.0f;
constexpr float kMonsterMeleeRangeY = 48.0f;
constexpr float kMonsterShootRangeX = 360.0f;
constexpr float kMonsterShootRangeY = 200.0f;

int monster_run_speed(map::MonsterType type) {
    switch (type) {
    case map::MonsterType::Zomby:
        return 2;
    case map::MonsterType::Imp:
    case map::MonsterType::Serg:
        return 3;
    case map::MonsterType::Demon:
    case map::MonsterType::Cgun:
        return 4;
    case map::MonsterType::Cyber:
    case map::MonsterType::Spider:
        return 1;
    default:
        return 3;
    }
}

bool monster_should_chase(map::MonsterType type) {
    switch (type) {
    case map::MonsterType::Barrel:
    case map::MonsterType::Fish:
        return false;
    default:
        return true;
    }
}

void publish_monster_damage(EventBus* events, int amount, int health_remaining) {
    if (events == nullptr || amount <= 0) {
        return;
    }
    events->publish(events::PlayerDamaged{amount, events::DamageSource::Melee, health_remaining});
    if (health_remaining <= 0) {
        events->publish(events::PlayerDied{events::DamageSource::Melee});
    }
}

void try_wake_monster(ShootableTarget& monster, bool should_chase, EventBus* events) {
    if (monster.is_awake || !should_chase) {
        return;
    }

    monster.is_awake = true;
    if (events == nullptr) {
        return;
    }

    const char* sfx = map::monster_alert_sfx(monster.monster_type, monster.id);
    if (sfx == nullptr) {
        return;
    }

    events->publish(events::MonsterAlerted{monster.id,
                                           static_cast<std::uint8_t>(monster.monster_type),
                                           monster.x,
                                           monster.y});
}

int z_dec(int value, int amount) {
    if (value > 0) {
        return std::max(0, value - amount);
    }
    if (value < 0) {
        return std::min(0, value + amount);
    }
    return 0;
}

int abs_int(int value) {
    return value < 0 ? -value : value;
}

bool monster_uses_gravity(const ShootableTarget& monster, const MapCollision& collision) {
    if (map::monster_is_flying(monster.monster_type)) {
        return false;
    }
    if (monster.monster_type == map::MonsterType::Fish) {
        return !collision.in_liquid(monster.x, monster.y, monster.width, monster.height);
    }
    return true;
}

void apply_vertical_physics(ShootableTarget& monster, const MapCollision& collision,
                            bool use_gravity) {
    switch (collision.vertical_lift_at(monster.x, monster.y, monster.width, monster.height)) {
    case -1:
        monster.vel_y -= 1;
        if (monster.vel_y < -5) {
            monster.vel_y += 1;
        }
        break;
    case 1:
        if (monster.vel_y > 5) {
            monster.vel_y -= 1;
        }
        monster.vel_y += 1;
        break;
    default:
        if (use_gravity) {
            monster.vel_y += 1;
            if (monster.vel_y > PlayerState::kMaxYVel) {
                monster.vel_y = PlayerState::kMaxYVel;
            }
        }
        break;
    }
}

void apply_horizontal_lift(ShootableTarget& monster, const MapCollision& collision) {
    switch (collision.horizontal_lift_at(monster.x, monster.y, monster.width, monster.height)) {
    case -1:
        monster.vel_x -= 3;
        if (monster.vel_x < -9) {
            monster.vel_x += 3;
        }
        break;
    case 1:
        monster.vel_x += 3;
        if (monster.vel_x > 9) {
            monster.vel_x -= 3;
        }
        break;
    default:
        break;
    }
}

void apply_water_damping(ShootableTarget& monster) {
    if (!monster.in_water) {
        return;
    }

    int damp_x = abs_int(monster.vel_x) + 1;
    if (damp_x > 5) {
        monster.vel_x = z_dec(monster.vel_x, (damp_x / 2) - 2);
    }
    int damp_y = abs_int(monster.vel_y) + 1;
    if (damp_y > 5) {
        monster.vel_y = z_dec(monster.vel_y, (damp_y / 2) - 2);
    }
}

void move_monster_body(ShootableTarget& monster, const MapCollision& collision,
                       bool climb_slopes) {
    float next_x = monster.x;
    float next_y = monster.y;
    const auto move_state =
        collision.move_monster(next_x, next_y, monster.width, monster.height, monster.vel_x,
                               monster.vel_y, climb_slopes);
    monster.x = next_x;
    monster.y = next_y;
    monster.in_water = (move_state & MOVE_INWATER) != 0;

    if ((move_state & MOVE_BLOCK) != 0) {
        monster.facing_left = !monster.facing_left;
        monster.vel_x = 0;
    }
    if ((move_state & MOVE_HITWALL) != 0) {
        monster.vel_x = 0;
    }
    if ((move_state & (MOVE_HITCEIL | MOVE_HITLAND)) != 0) {
        monster.vel_y = 0;
    }
}

void apply_monster_acid_damage(ShootableTarget& monster, const MapCollision& collision, int tick,
                               EventBus* events) {
    if (tick % PlayerState::kAcidDamagePeriod != 0) {
        return;
    }

    const int acid_damage =
        collision.acid_damage(monster.x, monster.y, monster.width, monster.height);
    if (acid_damage <= 0) {
        return;
    }

    const int health_before = monster.health;
    monster.apply_damage(acid_damage, 0);
    publish_entity_damage(events, monster.id, 0, health_before - monster.health,
                          events::DamageSource::Acid, monster.health);
}

bool monster_in_front(float monster_center_x, bool facing_left, float target_center_x) {
    if (target_center_x > monster_center_x) {
        return !facing_left;
    }
    if (target_center_x < monster_center_x) {
        return facing_left;
    }
    return true;
}

bool monster_has_line_of_sight(const MapCollision& collision, float monster_center_x,
                               float monster_center_y, float player_center_x,
                               float player_center_y) {
    const float dx = player_center_x - monster_center_x;
    const float dy = player_center_y - monster_center_y;
    const float player_dist_sq = dx * dx + dy * dy;
    if (player_dist_sq <= 0.0f) {
        return true;
    }

    const auto hit =
        collision.trace_solid_ray(monster_center_x, monster_center_y, player_center_x,
                                  player_center_y);
    if (!hit.hit) {
        return true;
    }
    return hit.distance_sq >= player_dist_sq - 4.0f;
}

bool monster_can_see_player(const MapCollision& collision, const ShootableTarget& monster,
                            float player_center_x, float player_center_y) {
    const float monster_center_x = monster.x + monster.width * 0.5f;
    const float monster_center_y = monster.y + monster.height * 0.5f;
    if (!monster_in_front(monster_center_x, monster.facing_left, player_center_x)) {
        return false;
    }
    return monster_has_line_of_sight(collision, monster_center_x, monster_center_y,
                                     player_center_x, player_center_y);
}

void apply_horizontal_chase(ShootableTarget& monster, float dx, int run_speed) {
    if (dx > 8.0f) {
        monster.vel_x = std::min(monster.vel_x + 1, run_speed);
        monster.facing_left = false;
    } else if (dx < -8.0f) {
        monster.vel_x = std::max(monster.vel_x - 1, -run_speed);
        monster.facing_left = true;
    } else {
        monster.vel_x = z_dec(monster.vel_x, 1);
    }
}

void tick_flying_monster(ShootableTarget& monster, const MapCollision& collision,
                         float dx, float dy, bool should_chase) {
    if (should_chase) {
        const int run_speed = monster_run_speed(monster.monster_type);
        apply_horizontal_chase(monster, dx, run_speed);

        if (dy > 8.0f) {
            monster.vel_y = std::min(monster.vel_y + 1, run_speed);
        } else if (dy < -8.0f) {
            monster.vel_y = std::max(monster.vel_y - 1, -run_speed);
        } else {
            monster.vel_y = z_dec(monster.vel_y, 1);
        }
    } else {
        monster.vel_x = z_dec(monster.vel_x, 1);
        monster.vel_y = z_dec(monster.vel_y, 1);
    }

    apply_vertical_physics(monster, collision, false);
    apply_horizontal_lift(monster, collision);
    apply_water_damping(monster);
    move_monster_body(monster, collision, false);
}

void tick_ground_monster(ShootableTarget& monster, const MapCollision& collision, float dx,
                         bool should_chase) {
    if (should_chase) {
        apply_horizontal_chase(monster, dx, monster_run_speed(monster.monster_type));
    } else {
        monster.vel_x = z_dec(monster.vel_x, 1);
    }

    const bool use_gravity = monster_uses_gravity(monster, collision);
    apply_vertical_physics(monster, collision, use_gravity);
    apply_horizontal_lift(monster, collision);
    apply_water_damping(monster);
    move_monster_body(monster, collision, true);
}

bool try_shoot_monster(ShootableTarget& monster, float player_center_x, float player_center_y,
                       float dx, float dy, bool can_see, ProjectileSystem* projectiles) {
    if (projectiles == nullptr || !can_see || monster.shoot_cooldown > 0 ||
        !map::monster_can_shoot(monster.monster_type)) {
        return false;
    }

    if (std::abs(dx) > kMonsterShootRangeX || std::abs(dy) > kMonsterShootRangeY) {
        return false;
    }

    if (std::abs(dx) < kMonsterMeleeRangeX && std::abs(dy) < kMonsterMeleeRangeY) {
        return false;
    }

    const float muzzle_x = monster.x + monster.width * 0.5f;
    const float muzzle_y = monster.y + monster.height * 0.5f;
    projectiles->spawn_monster_shot(muzzle_x, muzzle_y, player_center_x, player_center_y, monster.id);
    monster.shoot_cooldown = map::monster_shoot_cooldown_ticks(monster.monster_type);
    return true;
}

ShootableTarget make_spawned_soul(EntityId id, float spawn_x, float spawn_y, bool aggro_player) {
    const auto stats = map::monster_stats(map::MonsterType::Soul);
    ShootableTarget soul;
    soul.id = id;
    soul.monster_type = map::MonsterType::Soul;
    soul.x = spawn_x - stats.width * 0.5f;
    soul.y = spawn_y;
    soul.prev_x = soul.x;
    soul.prev_y = soul.y;
    soul.width = stats.width;
    soul.height = stats.height;
    soul.max_health = stats.health;
    soul.health = stats.health;
    soul.aggro_player = aggro_player;
    soul.is_awake = aggro_player;
    return soul;
}

void spawn_monster_soul(std::vector<ShootableTarget>& targets, EntityId& next_id, float spawn_x,
                        float spawn_y, bool aggro_player) {
    targets.push_back(make_spawned_soul(next_id++, spawn_x, spawn_y, aggro_player));
}

bool try_pain_spawn_soul(ShootableTarget& pain, float /*player_center_x*/, float /*player_center_y*/,
                         float dx, float dy, bool can_see, std::vector<ShootableTarget>& targets,
                         EntityId& next_id) {
    if (pain.monster_type != map::MonsterType::Pain || !can_see || pain.shoot_cooldown > 0) {
        return false;
    }

    if (std::abs(dx) > kMonsterShootRangeX || std::abs(dy) > kMonsterShootRangeY) {
        return false;
    }

    if (std::abs(dx) < kMonsterMeleeRangeX && std::abs(dy) < kMonsterMeleeRangeY) {
        return false;
    }

    const float spawn_x = pain.x + pain.width * 0.5f;
    const float spawn_y = pain.y;
    spawn_monster_soul(targets, next_id, spawn_x, spawn_y, true);
    pain.shoot_cooldown = map::monster_shoot_cooldown_ticks(map::MonsterType::Pain);
    return true;
}

constexpr int kReviveDurationTicks = 54;
constexpr int kVileReviveCooldown = 36;

void tick_passive_monster(ShootableTarget& monster) {
    if (!monster.is_monster()) {
        return;
    }

    if (monster.is_reviving()) {
        ++monster.anim_tick;
        --monster.revive_ticks;
        if (monster.revive_ticks <= 0) {
            monster.life_state = MonsterLifeState::Alive;
            monster.health = monster.max_health;
            monster.death_handled = false;
            monster.vel_x = 0;
            monster.vel_y = 0;
        }
        return;
    }

    if (monster.is_corpse()) {
        return;
    }
}

bool try_vile_revive(ShootableTarget& vile, std::vector<ShootableTarget>& targets, int tick) {
    if (vile.monster_type != map::MonsterType::Vile || vile.melee_cooldown > 0 || !vile.alive()) {
        return false;
    }

    for (auto& target : targets) {
        if (target.life_state != MonsterLifeState::Corpse ||
            !map::monster_can_be_revived(target.monster_type)) {
            continue;
        }

        if (!rects_overlap(vile.x, vile.y, vile.width, vile.height, target.x, target.y, target.width,
                           target.height)) {
            continue;
        }

        if ((tick + static_cast<int>(vile.id)) % 8 != 0) {
            continue;
        }

        target.life_state = MonsterLifeState::Reviving;
        target.revive_ticks = kReviveDurationTicks;
        vile.melee_cooldown = kVileReviveCooldown;
        return true;
    }

    return false;
}

void tick_monster(ShootableTarget& monster, const MapCollision& collision, PlayerState& player,
                  EventBus* events, ProjectileSystem* projectiles,
                  std::vector<ShootableTarget>& targets, EntityId& next_monster_id) {
    if (!monster.is_monster()) {
        return;
    }

    if (monster.is_corpse() || monster.is_reviving()) {
        return;
    }

    if (!monster.alive()) {
        return;
    }

    monster.prev_x = monster.x;
    monster.prev_y = monster.y;

    if (monster.melee_cooldown > 0) {
        --monster.melee_cooldown;
    }
    if (monster.shoot_cooldown > 0) {
        --monster.shoot_cooldown;
    }

    const float player_center_x = player.x + player.width * 0.5f;
    const float player_center_y = player.y + player.height * 0.5f;
    const float monster_center_x = monster.x + monster.width * 0.5f;
    const float monster_center_y = monster.y + monster.height * 0.5f;
    const float dx = player_center_x - monster_center_x;
    const float dy = player_center_y - monster_center_y;

    const bool player_alive = player.alive();
    const bool in_range = player_alive && monster_should_chase(monster.monster_type) &&
                          std::abs(dx) < kMonsterShootRangeX &&
                          std::abs(dy) < kMonsterShootRangeY;
    const bool can_see =
        in_range && monster_can_see_player(collision, monster, player_center_x, player_center_y);
    const bool should_chase = in_range && (can_see || monster.aggro_player);
    try_wake_monster(monster, should_chase, events);

    if (try_pain_spawn_soul(monster, player_center_x, player_center_y, dx, dy, can_see, targets,
                            next_monster_id)) {
        ++monster.anim_tick;
        return;
    }

    if (try_vile_revive(monster, targets, player.tick())) {
        ++monster.anim_tick;
        return;
    }

    if (try_shoot_monster(monster, player_center_x, player_center_y, dx, dy, can_see, projectiles)) {
        ++monster.anim_tick;
        return;
    }

    if (map::monster_is_flying(monster.monster_type)) {
        tick_flying_monster(monster, collision, dx, dy, should_chase);
    } else {
        tick_ground_monster(monster, collision, dx, should_chase);
    }

    apply_monster_acid_damage(monster, collision, player.tick(), events);

    if (!player_alive || monster.melee_cooldown > 0) {
        ++monster.anim_tick;
        return;
    }

    if (!rects_overlap(monster.x, monster.y, monster.width, monster.height, player.x, player.y,
                       player.width, player.height)) {
        ++monster.anim_tick;
        return;
    }

    const int health_before = player.health();
    const bool died = player.apply_damage(kMonsterMeleeDamage);
    publish_monster_damage(events, health_before - player.health(), player.health());
    monster.melee_cooldown = kMonsterMeleeCooldown;
    ++monster.anim_tick;
    (void)died;
}

} // namespace

void MonsterSystem::tick(const MapCollision& collision, PlayerState& player,
                         std::vector<ShootableTarget>& targets, EventBus* events,
                         ProjectileSystem* projectiles, EntityId* next_monster_id) {
    EntityId local_next_id = 0;
    EntityId& next_id = next_monster_id != nullptr ? *next_monster_id : local_next_id;
    for (auto& target : targets) {
        tick_passive_monster(target);
    }
    for (auto& target : targets) {
        tick_monster(target, collision, player, events, projectiles, targets, next_id);
    }
}

} // namespace d2df::sim
