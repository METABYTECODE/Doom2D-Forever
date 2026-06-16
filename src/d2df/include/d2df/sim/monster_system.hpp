#pragma once

#include <d2df/sim/map_collision.hpp>
#include <d2df/sim/player_state.hpp>
#include <d2df/sim/projectile_system.hpp>
#include <d2df/sim/shootable_target.hpp>

#include <vector>

namespace d2df {

class EventBus;

} // namespace d2df

namespace d2df::sim {

class MonsterSystem {
public:
    void tick(const MapCollision& collision, PlayerState& player, std::vector<ShootableTarget>& targets,
              EventBus* events, ProjectileSystem* projectiles = nullptr, EntityId* next_monster_id = nullptr);
};

} // namespace d2df::sim
