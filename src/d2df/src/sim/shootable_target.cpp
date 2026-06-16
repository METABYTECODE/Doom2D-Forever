#include <d2df/sim/shootable_target.hpp>

#include <algorithm>

namespace d2df::sim {

bool ShootableTarget::apply_damage(int amount) {
    if (amount <= 0 || !alive()) {
        return false;
    }
    health = std::max(0, health - amount);
    return !alive();
}

} // namespace d2df::sim
