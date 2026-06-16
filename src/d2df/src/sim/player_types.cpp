#include <d2df/sim/player_types.hpp>

#include <algorithm>
#include <cmath>

namespace d2df::sim {
namespace {

PlayerSprite make_sprite(const char* texture_id, int frame_count = 1, int anim_period = 0) {
    return {texture_id, 64, 64, frame_count, anim_period};
}

bool is_firing_anim(WeaponId weapon, bool firing) {
    return firing && weapon != WeaponId::Saw;
}

} // namespace

PlayerAnim resolve_player_anim(bool on_ground, int vel_x, bool aim_up, bool aim_down, bool firing,
                               WeaponId weapon) {
    const bool show_attack = is_firing_anim(weapon, firing);

    if (show_attack) {
        if (aim_up) {
            return PlayerAnim::AttackUp;
        }
        if (aim_down) {
            return PlayerAnim::AttackDown;
        }
        return PlayerAnim::Attack;
    }

    if (aim_up) {
        return PlayerAnim::SeeUp;
    }
    if (aim_down) {
        return PlayerAnim::SeeDown;
    }

    if (on_ground && vel_x != 0) {
        return PlayerAnim::Walk;
    }

    return PlayerAnim::Stand;
}

PlayerSprite player_sprite(PlayerAnim anim) {
    switch (anim) {
    case PlayerAnim::Stand:
        return make_sprite("tex.tile.stand");
    case PlayerAnim::Walk:
        // Doomer model: 4 frames, waitcount=8 -> speed 2 ticks per frame.
        return make_sprite("tex.tile.walk", 4, 2);
    case PlayerAnim::Attack:
        return make_sprite("tex.tile.attack");
    case PlayerAnim::SeeUp:
        return make_sprite("tex.tile.seeup");
    case PlayerAnim::SeeDown:
        return make_sprite("tex.tile.seedown");
    case PlayerAnim::AttackUp:
        return make_sprite("tex.tile.attackup");
    case PlayerAnim::AttackDown:
        return make_sprite("tex.tile.attackdown");
    default:
        return make_sprite("tex.tile.stand");
    }
}

PlayerSpritePlacement player_sprite_placement(float hitbox_x, float hitbox_y, bool facing_left) {
    const float object_x = hitbox_x - kPlayerSpriteOffsetX;
    const float object_y = hitbox_y - kPlayerSpriteOffsetY;

    if (!facing_left) {
        return {object_x, object_y, false};
    }

    const float anchor = kPlayerSpriteOffsetX + PlayerState::width;
    const float draw_x = object_x + kPlayerSpriteWidth - anchor - kPlayerSpriteOffsetX;
    return {draw_x, object_y, true};
}

int player_walk_frame_index(int tick, int vel_x) {
    if (std::abs(vel_x) < 4) {
        return 0;
    }
    return (tick / 2) % 4;
}

} // namespace d2df::sim
