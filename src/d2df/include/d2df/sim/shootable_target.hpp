#pragma once

#include <d2df/core/types.hpp>
#include <d2df/map/monster_types.hpp>

#include <cstdint>

namespace d2df::sim {

enum class MonsterLifeState : std::uint8_t {
    Alive = 0,
    Corpse = 1,
    Reviving = 2,
};

/// Damageable map entity (monster placeholder or debug target).
struct ShootableTarget {
    EntityId id = 0;
    map::MonsterType monster_type = map::MonsterType::None;
    float x = 0.0f;
    float y = 0.0f;
    float prev_x = 0.0f;
    float prev_y = 0.0f;
    float width = 34.0f;
    float height = 52.0f;
    int vel_x = 0;
    int vel_y = 0;
    int health = 100;
    int max_health = 100;
    int melee_cooldown = 0;
    int shoot_cooldown = 0;
    int anim_tick = 0;
    int revive_ticks = 0;
    bool facing_left = false;
    bool aggro_player = false;
    bool in_water = false;
    bool death_handled = false;
    MonsterLifeState life_state = MonsterLifeState::Alive;

    [[nodiscard]] bool alive() const {
        return health > 0 && life_state == MonsterLifeState::Alive;
    }
    [[nodiscard]] bool is_corpse() const { return life_state == MonsterLifeState::Corpse; }
    [[nodiscard]] bool is_reviving() const { return life_state == MonsterLifeState::Reviving; }
    [[nodiscard]] bool is_monster() const { return monster_type != map::MonsterType::None; }
    [[nodiscard]] float render_x(float alpha) const;
    [[nodiscard]] float render_y(float alpha) const;

    /// @return true if the target died from this hit.
    bool apply_damage(int amount, EntityId attacker_id = 0);
};

} // namespace d2df::sim
