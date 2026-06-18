#pragma once

#include <d2df/sim/player_state.hpp>

namespace d2df::engine::physics {

/// Push the player out of overlapping solid entity boxes (monsters, props).
void resolve_player_entity_collisions(sim::PlayerState& player, float entity_x, float entity_y,
                                      float entity_width, float entity_height);

} // namespace d2df::engine::physics
