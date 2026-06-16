#pragma once

#include <d2df/sim/projectile_system.hpp>

namespace d2df::sim {

struct ProjectileSprite {
    const char* texture_id = nullptr;
    /// Sub-rectangle inside the texture; 0 means use the full texture width/height.
    int frame_width = 0;
    int frame_height = 0;
    int frame_count = 1;
    /// Ticks per animation frame; 0 means static.
    int anim_period = 0;
    float draw_offset_x = 0.0f;
    float draw_offset_y = 0.0f;
    bool rotate_to_velocity = false;
};

[[nodiscard]] ProjectileSprite projectile_sprite(ProjectileKind kind);

} // namespace d2df::sim
