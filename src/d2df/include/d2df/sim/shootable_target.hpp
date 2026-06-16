#pragma once

#include <d2df/core/types.hpp>

namespace d2df::sim {

/// Simple damageable AABB for combat testing before monsters (Phase 6).
struct ShootableTarget {
    EntityId id = 0;
    float x = 0.0f;
    float y = 0.0f;
    float width = 34.0f;
    float height = 52.0f;
    int health = 100;
    int max_health = 100;

    [[nodiscard]] bool alive() const { return health > 0; }

    /// @return true if the target died from this hit.
    bool apply_damage(int amount);
};

} // namespace d2df::sim
