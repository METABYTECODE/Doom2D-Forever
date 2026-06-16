#pragma once

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
    static constexpr int kTrapDamage = 1000;
    static constexpr int kAcidDamagePeriod = 15;

    void snap_to(float spawn_x, float spawn_y);
    void begin_tick();
    void fixed_update(const MapCollision& collision, PlayerInput input);

    [[nodiscard]] bool on_ground() const { return on_ground_; }
    [[nodiscard]] bool in_water() const { return in_water_; }
    [[nodiscard]] bool in_acid() const { return in_acid_; }
    [[nodiscard]] bool on_lift() const { return on_lift_; }
    [[nodiscard]] bool alive() const { return health_ > 0; }
    [[nodiscard]] int health() const { return health_; }
    [[nodiscard]] int tick() const { return tick_; }
    [[nodiscard]] float render_x(float alpha) const;
    [[nodiscard]] float render_y(float alpha) const;

    /// @return true if the player died from this hit.
    bool apply_damage(int amount);

    void restore_health();
    /// Raise health up to cap; returns false if already at or above cap.
    bool add_health(int amount, int max_health);
    void set_health(int value);

    [[nodiscard]] PlayerCombat& combat() { return combat_; }
    [[nodiscard]] const PlayerCombat& combat() const { return combat_; }

private:
    void update_facing(bool left, bool right);
    void apply_run(bool left, bool right);
    void try_jump(const MapCollision& collision);
    static int z_dec(int value, int amount);

    int tick_ = 0;
    int health_ = kMaxHealth;
    bool on_ground_ = false;
    bool in_water_ = false;
    bool in_acid_ = false;
    bool on_lift_ = false;
    PlayerCombat combat_;
};

} // namespace d2df::sim
