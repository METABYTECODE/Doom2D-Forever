#pragma once

#include <d2df/map/map_document.hpp>
#include <d2df/sim/map_collision.hpp>
#include <d2df/sim/player_state.hpp>
#include <d2df/sim/projectile_system.hpp>
#include <d2df/sim/shootable_target.hpp>
#include <d2df/sim/trigger_system.hpp>

#include <vector>

namespace d2df::debug {

struct DebugWorldView {
    const sim::PlayerState& player;
    const std::vector<sim::ShootableTarget>& targets;
    const sim::ProjectileSystem& projectiles;
    const sim::MapCollision& collision;
    const map::MapDocument& map;
    const sim::TriggerSystem& triggers;
    float render_alpha = 1.0f;
};

} // namespace d2df::debug
