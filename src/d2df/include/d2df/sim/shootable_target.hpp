#pragma once

#include <d2df/core/types.hpp>
#include <d2df/map/monster_types.hpp>

namespace d2df::sim {

/// Damageable map entity (monster placeholder or debug target).
struct ShootableTarget {
    EntityId id = 0;
    map::MonsterType monster_type = map::MonsterType::None;
    float x = 0.0f;
    float y = 0.0f;
    float width = 34.0f;
    float height = 52.0f;
    int health = 100;
    int max_health = 100;

    [[nodiscard]] bool alive() const { return health > 0; }
    [[nodiscard]] bool is_monster() const { return monster_type != map::MonsterType::None; }

    /// @return true if the target died from this hit.
    bool apply_damage(int amount);
};

} // namespace d2df::sim
