#include <d2df/sim/player_types.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace d2df::sim {
namespace {

PlayerSprite make_sprite(const char* texture_id, int frame_count = 1, int anim_period = 0) {
    return {texture_id, 64, 64, frame_count, anim_period};
}

PlayerSpriteSet make_set(const char* body_id, const char* mask_id, int frame_count = 1,
                         int anim_period = 0) {
    return {make_sprite(body_id, frame_count, anim_period), mask_id};
}

bool is_firing_anim(WeaponId weapon, bool firing) {
    return firing && weapon != WeaponId::Saw;
}

enum class WeaponPose {
    Normal,
    Up,
    Down,
};

WeaponPose weapon_pose_for_anim(PlayerAnim anim) {
    switch (anim) {
    case PlayerAnim::SeeUp:
    case PlayerAnim::AttackUp:
        return WeaponPose::Up;
    case PlayerAnim::SeeDown:
    case PlayerAnim::AttackDown:
        return WeaponPose::Down;
    default:
        return WeaponPose::Normal;
    }
}

const char* weapon_base_name(WeaponId weapon) {
    switch (weapon) {
    case WeaponId::Saw:
        return "csaw";
    case WeaponId::Pistol:
        return "hgun";
    case WeaponId::Shotgun1:
        return "sg";
    case WeaponId::Shotgun2:
        return "ssg";
    case WeaponId::Chaingun:
        return "mgun";
    case WeaponId::RocketLauncher:
        return "rkt";
    case WeaponId::Plasma:
        return "plz";
    case WeaponId::Bfg:
        return "bfg";
    case WeaponId::SuperChaingun:
        return "spl";
    case WeaponId::Flamethrower:
        return "flm";
    default:
        return nullptr;
    }
}

const char* weapon_guns2_normal_suffix(WeaponId weapon) {
    switch (weapon) {
    case WeaponId::Chaingun:
    case WeaponId::Bfg:
        return "_3";
    default:
        return "_2";
    }
}

} // namespace

const char* player_weapon_texture_id(WeaponId weapon, PlayerAnim anim, bool firing) {
    const char* base = weapon_base_name(weapon);
    if (base == nullptr) {
        return nullptr;
    }

    static thread_local char buffer[64];
    const WeaponPose pose = weapon_pose_for_anim(anim);

    switch (pose) {
    case WeaponPose::Up:
        if (firing) {
            std::snprintf(buffer, sizeof(buffer), "tex.weapon.%s_up_fire_2", base);
        } else {
            std::snprintf(buffer, sizeof(buffer), "tex.weapon.%s_up_2", base);
        }
        break;
    case WeaponPose::Down:
        if (firing) {
            std::snprintf(buffer, sizeof(buffer), "tex.weapon.%s_dn_fire_2", base);
        } else {
            std::snprintf(buffer, sizeof(buffer), "tex.weapon.%s_dn_2", base);
        }
        break;
    case WeaponPose::Normal:
        if (firing) {
            std::snprintf(buffer, sizeof(buffer), "tex.weapon.%s_fire_2", base);
        } else {
            std::snprintf(buffer, sizeof(buffer), "tex.weapon.%s%s", base,
                          weapon_guns2_normal_suffix(weapon));
        }
        break;
    }

    return buffer;
}

PlayerAnim resolve_player_anim(bool on_ground, int vel_x, bool aim_up, bool aim_down, bool firing,
                               WeaponId weapon, int pain_ticks, PlayerDeathPhase death_phase) {
    if (death_phase == PlayerDeathPhase::Die2) {
        return PlayerAnim::Die2;
    }
    if (death_phase == PlayerDeathPhase::Die1) {
        return PlayerAnim::Die1;
    }
    if (pain_ticks > 0) {
        return PlayerAnim::Pain;
    }

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

PlayerSpriteSet player_sprite_set(PlayerAnim anim) {
    switch (anim) {
    case PlayerAnim::Stand:
        return make_set("tex.tile.stand", "tex.tile.standmask");
    case PlayerAnim::Walk:
        return make_set("tex.tile.walk", "tex.tile.walkmask", 4, 2);
    case PlayerAnim::Attack:
        return make_set("tex.tile.attack", "tex.tile.attackmask");
    case PlayerAnim::SeeUp:
        return make_set("tex.tile.seeup", "tex.tile.seeupmask");
    case PlayerAnim::SeeDown:
        return make_set("tex.tile.seedown", "tex.tile.seedownmask");
    case PlayerAnim::AttackUp:
        return make_set("tex.tile.attackup", "tex.tile.attackupmask");
    case PlayerAnim::AttackDown:
        return make_set("tex.tile.attackdown", "tex.tile.attackdownmask");
    case PlayerAnim::Pain:
        return make_set("tex.tile.pain", "tex.tile.painmask");
    case PlayerAnim::Die1:
        return make_set("tex.tile.die1", "tex.tile.die1mask", 6, 3);
    case PlayerAnim::Die2:
        return make_set("tex.tile.die2", "tex.tile.die2mask");
    default:
        return make_set("tex.tile.stand", "tex.tile.standmask");
    }
}

bool should_draw_weapon(WeaponId weapon, PlayerAnim anim) {
    if (weapon == WeaponId::Knuckles) {
        return false;
    }
    if (anim == PlayerAnim::Pain || anim == PlayerAnim::Die1 || anim == PlayerAnim::Die2) {
        return false;
    }
    return weapon_base_name(weapon) != nullptr;
}

namespace {

struct WeaponPoint {
    int dx = 0;
    int dy = 0;
};

static constexpr WeaponPoint kStandWeaponPoints[1][10] = {
    {{30, 2}, {41, 0}, {22, 0}, {23, 9}, {23, 0}, {16, 0}, {26, 1}, {23, 1}, {22, 0}, {24, 2}},
};

static constexpr WeaponPoint kWalkWeaponPoints[4][10] = {
    {{28, 1}, {41, -1}, {22, 0}, {24, 8}, {22, -2}, {16, -1}, {25, 0}, {23, 0}, {22, -2}, {23, 0}},
    {{29, 0}, {42, -2}, {23, -1}, {25, 7}, {23, -3}, {17, -2}, {26, -1}, {24, -1}, {23, -3}, {24, -1}},
    {{30, 2}, {43, 0}, {24, 1}, {26, 9}, {24, -1}, {18, 0}, {27, 1}, {25, 1}, {24, -1}, {25, 1}},
    {{27, 1}, {40, -1}, {21, 0}, {23, 8}, {21, -2}, {15, -1}, {24, 0}, {22, 0}, {21, -2}, {22, 0}},
};

static constexpr WeaponPoint kAttackWeaponPoints[1][10] = {
    {{30, 2}, {40, 0}, {21, 0}, {22, 9}, {23, 0}, {15, 0}, {25, 1}, {22, 1}, {20, 0}, {23, 2}},
};

static constexpr WeaponPoint kSeeUpWeaponPoints[1][10] = {
    {{32, -6}, {23, -32}, {25, -14}, {24, -5}, {15, -17}, {17, -13}, {19, -18}, {21, -16}, {16, -17}, {19, -15}},
};

static constexpr WeaponPoint kSeeDownWeaponPoints[1][10] = {
    {{31, 1}, {42, 19}, {26, 15}, {24, 21}, {28, 8}, {22, 6}, {28, 8}, {27, 11}, {28, 7}, {26, 9}},
};

static constexpr WeaponPoint kAttackUpWeaponPoints[1][10] = {
    {{31, -6}, {22, -32}, {23, -14}, {23, -5}, {14, -17}, {17, -13}, {18, -18}, {20, -16}, {15, -18}, {18, -15}},
};

static constexpr WeaponPoint kAttackDownWeaponPoints[1][10] = {
    {{30, 1}, {41, 20}, {26, 15}, {23, 21}, {28, 8}, {21, 5}, {27, 8}, {26, 11}, {28, 7}, {25, 9}},
};

struct WeaponPointAnimTable {
    PlayerAnim anim = PlayerAnim::Stand;
    const WeaponPoint (*frames)[10] = nullptr;
    int frame_count = 0;
};

static constexpr WeaponPointAnimTable kWeaponPointTables[] = {
    {PlayerAnim::Stand, kStandWeaponPoints, 1},
    {PlayerAnim::Walk, kWalkWeaponPoints, 4},
    {PlayerAnim::Attack, kAttackWeaponPoints, 1},
    {PlayerAnim::SeeUp, kSeeUpWeaponPoints, 1},
    {PlayerAnim::SeeDown, kSeeDownWeaponPoints, 1},
    {PlayerAnim::AttackUp, kAttackUpWeaponPoints, 1},
    {PlayerAnim::AttackDown, kAttackDownWeaponPoints, 1},
};

const WeaponPointAnimTable* weapon_point_table_for_anim(PlayerAnim anim) {
    for (const auto& table : kWeaponPointTables) {
        if (table.anim == anim) {
            return &table;
        }
    }
    return nullptr;
}

int weapon_point_index(WeaponId weapon) {
    const int index = static_cast<int>(weapon) - static_cast<int>(WeaponId::Saw);
    if (index < 0 || index >= 10) {
        return -1;
    }
    return index;
}

} // namespace

bool player_weapon_draw_offset(WeaponId weapon, PlayerAnim anim, int frame_index, bool facing_left,
                               PlayerWeaponDrawOffset& out) {
    const int weapon_index = weapon_point_index(weapon);
    if (weapon_index < 0) {
        return false;
    }

    const WeaponPointAnimTable* table = weapon_point_table_for_anim(anim);
    if (table == nullptr || table->frame_count <= 0) {
        return false;
    }

    const int resolved_frame =
        std::clamp(frame_index, 0, std::max(0, table->frame_count - 1));
    const WeaponPoint& point = table->frames[resolved_frame][weapon_index];

    out.dx = static_cast<float>(facing_left ? -point.dx : point.dx);
    out.dy = static_cast<float>(point.dy);
    return true;
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

int player_anim_frame_index(PlayerAnim anim, int tick, int vel_x) {
    if (anim == PlayerAnim::Walk) {
        return player_walk_frame_index(tick, vel_x);
    }

    const auto set = player_sprite_set(anim);
    if (set.body.frame_count <= 1 || set.body.anim_period <= 0) {
        return 0;
    }
    return (tick / set.body.anim_period) % set.body.frame_count;
}

PlayerTeamColor default_player_team_color() {
    return {};
}

} // namespace d2df::sim
