#pragma once

#include <d2df/core/game_events.hpp>
#include <d2df/sim/projectile_system.hpp>

#include <optional>

namespace d2df::sim {

struct ExplosionSprite {
    const char* texture_id = nullptr;
    int frame_width = 0;
    int frame_height = 0;
    int frame_count = 1;
    int anim_period = 0;
    float draw_offset_x = 0.0f;
    float draw_offset_y = 0.0f;
};

[[nodiscard]] ExplosionSprite explosion_sprite(events::ExplosionKind kind);
[[nodiscard]] std::optional<events::ExplosionKind> explosion_kind_for_projectile(ProjectileKind kind);
[[nodiscard]] const char* explosion_sfx(events::ExplosionKind kind);

} // namespace d2df::sim
