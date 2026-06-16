#pragma once

#include <d2df/sim/player_state.hpp>
#include <d2df/sim/weapon_types.hpp>

namespace d2df::sim {

enum class PlayerAnim {
    Stand,
    Walk,
    Attack,
    SeeUp,
    SeeDown,
    AttackUp,
    AttackDown,
};

struct PlayerSprite {
    const char* texture_id = nullptr;
    int frame_width = 64;
    int frame_height = 64;
    int frame_count = 1;
    /// Ticks per animation frame; 0 means static.
    int anim_period = 0;
};

struct PlayerSpritePlacement {
    float x = 0.0f;
    float y = 0.0f;
    bool flip_horizontal = false;
};

/// Legacy PLAYER_RECT origin inside the 64x64 sprite.
constexpr float kPlayerSpriteOffsetX = 15.0f;
constexpr float kPlayerSpriteOffsetY = 12.0f;
constexpr float kPlayerSpriteWidth = 64.0f;
constexpr float kPlayerSpriteHeight = 64.0f;

[[nodiscard]] PlayerAnim resolve_player_anim(bool on_ground, int vel_x, bool aim_up, bool aim_down,
                                             bool firing, WeaponId weapon);

[[nodiscard]] PlayerSprite player_sprite(PlayerAnim anim);

[[nodiscard]] PlayerSpritePlacement player_sprite_placement(float hitbox_x, float hitbox_y,
                                                            bool facing_left);

[[nodiscard]] int player_walk_frame_index(int tick, int vel_x);

} // namespace d2df::sim
