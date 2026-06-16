#pragma once

#include <cstdint>

#include <d2df/sim/weapon_types.hpp>

namespace d2df::sim {

class MapCollision;

struct PlayerInput {
    bool left = false;
    bool right = false;
    bool jump = false;
    bool aim_up = false;
    bool aim_down = false;
    bool fire = false;
    bool weapon_prev = false;
    bool weapon_next = false;
    /// -1 = no direct selection this tick; otherwise WeaponId index.
    int weapon_select_request = -1;
};

/// Player hitbox matches legacy PLAYER_RECT (34x52 at spawn point).
struct PlayerState {
    float x = 0.0f;
    float y = 0.0f;
    float prev_x = 0.0f;
    float prev_y = 0.0f;

    int vel_x = 0;
    int vel_y = 0;
    int accel_x = 0;
    int accel_y = 0;

    static constexpr float width = 34.0f;
    static constexpr float height = 52.0f;

    static constexpr int kMaxRunVel = 8;
    static constexpr int kMaxYVel = 30;
    static constexpr int kVelJump = 10;
    static constexpr int kVelSwim = 4;

    static constexpr int kMaxHealth = 100;
    static constexpr int kHealthLimit = 200;
    static constexpr int kArmorSoftCap = 100;
    static constexpr int kArmorLimit = 200;
    static constexpr int kTrapDamage = 1000;
    static constexpr int kAcidDamagePeriod = 15;
    static constexpr int kTicksPerSecond = 36;

    static constexpr int kPowerupSuitTicks = 30 * kTicksPerSecond;
    static constexpr int kPowerupInvulTicks = 30 * kTicksPerSecond;
    static constexpr int kPowerupInvisTicks = 35 * kTicksPerSecond;
    static constexpr int kBerserkTicks = 30 * kTicksPerSecond;
    static constexpr int kAirMax = 1091;
    static constexpr int kJetFuelMax = 540;
    static constexpr int kVelFly = 6;

    PlayerState();

    void snap_to(float x, float y);
    void reset_to_spawn(float spawn_x, float spawn_y);
    void begin_tick();
    void fixed_update(const MapCollision& collision, PlayerInput input);

    [[nodiscard]] bool on_ground() const { return on_ground_; }
    [[nodiscard]] bool in_water() const { return in_water_; }
    [[nodiscard]] bool in_acid() const { return in_acid_; }
    [[nodiscard]] bool on_lift() const { return on_lift_; }
    [[nodiscard]] bool alive() const { return health_ > 0; }
    [[nodiscard]] int health() const { return health_; }
    [[nodiscard]] int armor() const { return armor_; }
    [[nodiscard]] bool has_key_red() const { return key_red_; }
    [[nodiscard]] bool has_key_green() const { return key_green_; }
    [[nodiscard]] bool has_key_blue() const { return key_blue_; }
    [[nodiscard]] std::uint8_t key_mask() const;
    [[nodiscard]] int tick() const { return tick_; }
    [[nodiscard]] float render_x(float alpha) const;
    [[nodiscard]] float render_y(float alpha) const;

    /// @return true if the player died from this hit.
    bool apply_damage(int amount);

    void restore_health();
    /// Raise health up to cap; returns false if already at or above cap.
    bool add_health(int amount, int max_health);
    void set_health(int value);
    void set_armor(int value);
  /// @return false if the player already has this key.
    bool give_key_red();
    bool give_key_green();
    bool give_key_blue();

    [[nodiscard]] bool has_suit() const;
    [[nodiscard]] bool has_invul() const;
    [[nodiscard]] bool has_invis() const;
    [[nodiscard]] bool has_berserk() const;
    [[nodiscard]] int jet_fuel() const { return jet_fuel_; }
    [[nodiscard]] int air() const { return air_; }
    [[nodiscard]] bool jetpack_active() const { return jetpack_active_; }

    void give_suit();
    void give_invul();
    void give_invis();
    void give_berserk();
    void refill_oxygen();
    void refill_jetpack();
    void heal_bottle();
    bool add_armor_small();

    [[nodiscard]] PlayerCombat& combat() { return combat_; }
    [[nodiscard]] const PlayerCombat& combat() const { return combat_; }

private:
    void update_facing(bool left, bool right);
    void apply_run(bool left, bool right);
    void try_jump(const MapCollision& collision);
    static int z_dec(int value, int amount);

    int tick_ = 0;
    int health_ = kMaxHealth;
    int armor_ = 0;
    bool key_red_ = false;
    bool key_green_ = false;
    bool key_blue_ = false;
    int powerup_suit_until_ = 0;
    int powerup_invul_until_ = 0;
    int powerup_invis_until_ = 0;
    int berserk_until_ = 0;
    int jet_fuel_ = 0;
    int air_ = kAirMax;
    bool jetpack_active_ = false;
    bool can_jetpack_ = false;
    bool on_ground_ = false;
    bool in_water_ = false;
    bool in_acid_ = false;
    bool on_lift_ = false;
    PlayerCombat combat_;
};

} // namespace d2df::sim
