#include <d2df/core/types.hpp>
#include <d2df/map/monster_types.hpp>
#include <d2df/sim/shootable_target.hpp>

#include <algorithm>

namespace d2df::sim {

float ShootableTarget::render_x(float alpha) const {
    return prev_x + (x - prev_x) * alpha;
}

float ShootableTarget::render_y(float alpha) const {
    return prev_y + (y - prev_y) * alpha;
}

bool ShootableTarget::apply_damage(int amount, EntityId attacker_id) {
    if (amount <= 0 || !alive()) {
        return false;
    }
    if (attacker_id == kPlayerEntityId) {
        aggro_player = true;
    }
    health = std::max(0, health - amount);
    if (health <= 0 && is_monster() && !map::monster_vanishes_on_death(monster_type)) {
        life_state = MonsterLifeState::Corpse;
        prev_x = x;
        prev_y = y;
        vel_x = 0;
        vel_y = 0;
    }
    return !alive();
}

} // namespace d2df::sim
