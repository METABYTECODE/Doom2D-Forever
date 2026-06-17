#pragma once

#include <d2df/sim/player_state.hpp>
#include <d2df/sim/weapon_types.hpp>

#include <cstdint>

namespace d2df::sim {

enum class PlayerAnim {
    Stand,
    Walk,
    Attack,
    SeeUp,
    SeeDown,
    AttackUp,
    AttackDown,
    Pain,
    Die1,
    Die2,
};

struct PlayerSprite {
    const char* texture_id = nullptr;
    int frame_width = 64;
    int frame_height = 64;
    int frame_count = 1;
    /// Ticks per animation frame; 0 means static.
    int anim_period = 0;
};

struct PlayerSpriteSet {
    PlayerSprite body{};
    const char* mask_texture_id = nullptr;
};

struct PlayerTeamColor {
    std::uint8_t r = 80;
    std::uint8_t g = 200;
    std::uint8_t b = 80;
};

struct PlayerSpritePlacement {
    float x = 0.0f;
    float y = 0.0f;
    bool flip_horizontal = false;
};

struct PlayerWeaponDrawOffset {
    float dx = 0.0f;
    float dy = 0.0f;
};

/// Legacy PLAYER_RECT origin inside the 64x64 sprite.
constexpr float kPlayerSpriteOffsetX = 15.0f;
constexpr float kPlayerSpriteOffsetY = 12.0f;
constexpr float kPlayerSpriteWidth = 64.0f;
constexpr float kPlayerSpriteHeight = 64.0f;

constexpr int kPlayerPainTicks = 18;
constexpr int kPlayerDie1Frames = 7;
constexpr int kPlayerDie1FramePeriod = 3;
constexpr int kPlayerDie2Frames = 9;
constexpr int kPlayerDie2FramePeriod = 2;
constexpr int kPlayerDieHardHealthThreshold = -30;
constexpr int kPlayerDie1Ticks = kPlayerDie1Frames * kPlayerDie1FramePeriod;
constexpr int kPlayerRespawnTicks = 54;
constexpr int kPlayerPunchFrames = 4;
constexpr float kPlayerCorpseHitboxOffsetX = 15.0f;
constexpr float kPlayerCorpseHitboxOffsetY = 48.0f;
constexpr float kPlayerCorpseHitboxWidth = 34.0f;
constexpr float kPlayerCorpseHitboxHeight = 16.0f;
constexpr float kPlayerPunchOffsetY = 11.0f;
constexpr int kPowerupFlickerStartTicks = 76;
constexpr int kPowerupFlickerPeriodTicks = 11;

struct PlayerOverlayTint {
    bool active = false;
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 0;
};

[[nodiscard]] PlayerAnim resolve_player_anim(bool on_ground, int vel_x, bool aim_up, bool aim_down,
                                             bool firing, WeaponId weapon, int pain_ticks,
                                             PlayerDeathPhase death_phase);

[[nodiscard]] PlayerSpriteSet player_sprite_set(PlayerAnim anim);

[[nodiscard]] const char* player_weapon_texture_id(WeaponId weapon, PlayerAnim anim, bool firing);

[[nodiscard]] bool should_draw_weapon(WeaponId weapon, PlayerAnim anim);

/// Weapon overlay offset from doomer model.bin (legacy GetWeapPoints + WEAPONBASE).
[[nodiscard]] bool player_weapon_draw_offset(WeaponId weapon, PlayerAnim anim, int frame_index,
                                             bool facing_left, PlayerWeaponDrawOffset& out);

[[nodiscard]] PlayerSpritePlacement player_sprite_placement(float hitbox_x, float hitbox_y,
                                                            bool facing_left);

[[nodiscard]] int player_walk_frame_index(int tick, int vel_x);

[[nodiscard]] int player_anim_frame_index(PlayerAnim anim, int tick, int vel_x);

[[nodiscard]] PlayerTeamColor default_player_team_color();

[[nodiscard]] const char* player_punch_texture_id(bool aim_up, bool aim_down, bool berserk);

[[nodiscard]] PlayerSpritePlacement player_punch_placement(float hitbox_x, float hitbox_y,
                                                           bool facing_left);

[[nodiscard]] int player_punch_frame_index(int punch_ticks);

[[nodiscard]] const char* player_model_pain_sfx(int damage_amount, int tick);

[[nodiscard]] const char* player_model_death_sfx();

[[nodiscard]] PlayerSpritePlacement player_invul_penta_placement(float hitbox_x, float hitbox_y,
                                                                 bool facing_left, int sprite_width,
                                                                 int sprite_height);

[[nodiscard]] bool powerup_flicker_visible(int ticks_remaining);

[[nodiscard]] std::uint8_t player_draw_alpha(const PlayerState& player);

[[nodiscard]] bool player_invul_penta_visible(const PlayerState& player);

[[nodiscard]] PlayerOverlayTint player_invul_overlay(const PlayerState& player);

[[nodiscard]] PlayerOverlayTint player_suit_overlay(const PlayerState& player);

[[nodiscard]] PlayerOverlayTint player_berserk_overlay(const PlayerState& player);

[[nodiscard]] PlayerOverlayTint player_pain_overlay(const PlayerState& player);

struct PlayerCorpseDraw {
    PlayerAnim anim = PlayerAnim::Die1;
    int frame_index = 0;
};

[[nodiscard]] PlayerCorpseDraw player_corpse_draw(bool mess);

[[nodiscard]] const char* player_gib_body_texture_id();

[[nodiscard]] const char* player_gib_mask_texture_id();

} // namespace d2df::sim
