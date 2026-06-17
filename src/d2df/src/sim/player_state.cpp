#include <d2df/sim/player_state.hpp>

#include <algorithm>
#include <cmath>

#include <d2df/map/panel_types.hpp>
#include <d2df/map/key_types.hpp>
#include <d2df/sim/game_save.hpp>
#include <d2df/sim/map_collision.hpp>
#include <d2df/sim/player_types.hpp>

namespace d2df::sim {
namespace {

int abs_int(int value) {
    return value < 0 ? -value : value;
}

} // namespace

PlayerState::PlayerState() {
    combat_.reset_single_player_loadout();
}

void PlayerState::snap_to(float pos_x, float pos_y) {
    x = pos_x;
    y = pos_y;
    prev_x = pos_x;
    prev_y = pos_y;
    vel_x = 0;
    vel_y = 0;
    accel_x = 0;
    accel_y = 0;
    on_ground_ = false;
    in_water_ = false;
    in_acid_ = false;
    on_lift_ = false;
    run_direction_ = 0;
}

void PlayerState::reset_to_spawn(float spawn_x, float spawn_y) {
    snap_to(spawn_x, spawn_y);
    tick_ = 0;
    health_ = kMaxHealth;
    combat_.reset_single_player_loadout();
    armor_ = 0;
    key_red_ = false;
    key_green_ = false;
    key_blue_ = false;
    has_backpack_ = false;
    powerup_suit_until_ = 0;
    powerup_invul_until_ = 0;
    powerup_invis_until_ = 0;
    berserk_until_ = 0;
    jet_fuel_ = 0;
    air_ = kAirMax;
    jetpack_active_ = false;
    can_jetpack_ = false;
    reset_death_state();
}

std::uint8_t PlayerState::key_mask() const {
    std::uint8_t mask = map::KeyNone;
    if (key_red_) {
        mask |= map::KeyRed;
    }
    if (key_green_) {
        mask |= map::KeyGreen;
    }
    if (key_blue_) {
        mask |= map::KeyBlue;
    }
    return mask;
}

bool PlayerState::has_suit() const {
    return tick_ < powerup_suit_until_;
}

bool PlayerState::has_invul() const {
    return tick_ < powerup_invul_until_;
}

bool PlayerState::has_invis() const {
    return tick_ < powerup_invis_until_;
}

bool PlayerState::has_berserk() const {
    return tick_ < berserk_until_;
}

int PlayerState::powerup_suit_ticks_remaining() const {
    return std::max(0, powerup_suit_until_ - tick_);
}

int PlayerState::powerup_invul_ticks_remaining() const {
    return std::max(0, powerup_invul_until_ - tick_);
}

int PlayerState::powerup_invis_ticks_remaining() const {
    return std::max(0, powerup_invis_until_ - tick_);
}

int PlayerState::berserk_ticks_remaining() const {
    return std::max(0, berserk_until_ - tick_);
}

void PlayerState::give_suit() {
    powerup_suit_until_ = std::max(powerup_suit_until_, tick_ + kPowerupSuitTicks);
}

void PlayerState::give_invul() {
    powerup_invul_until_ = std::max(powerup_invul_until_, tick_ + kPowerupInvulTicks);
}

void PlayerState::give_invis() {
    powerup_invis_until_ = std::max(powerup_invis_until_, tick_ + kPowerupInvisTicks);
}

void PlayerState::give_berserk() {
    berserk_until_ = std::max(berserk_until_, tick_ + kBerserkTicks);
}

void PlayerState::refill_oxygen() {
    air_ = kAirMax;
}

void PlayerState::refill_jetpack() {
    jet_fuel_ = kJetFuelMax;
}

void PlayerState::heal_bottle() {
    if (health_ < kHealthLimit) {
        health_ = std::min(health_ + 4, kHealthLimit);
    }
}

bool PlayerState::add_armor_small() {
    if (armor_ >= kArmorLimit) {
        return false;
    }
    armor_ = std::min(armor_ + 5, kArmorLimit);
    return true;
}

bool PlayerState::apply_damage(int amount) {
    if (amount <= 0 || !alive() || has_invul()) {
        return false;
    }

    if (amount >= 9) {
        pain_ticks_ = kPlayerPainTicks;
    }

    int remaining = amount;
    if (armor_ > 0) {
        const int absorbed = std::min(armor_, remaining);
        armor_ -= absorbed;
        remaining -= absorbed;
    }

    if (remaining <= 0) {
        return false;
    }

    health_ -= remaining;
    if (health_ <= 0) {
        death_health_ = health_;
        health_ = 0;
        pain_ticks_ = 0;
        death_phase_ = death_health_ <= kPlayerDieHardHealthThreshold ? PlayerDeathPhase::Die2
                                                                      : PlayerDeathPhase::Die1;
        death_started_tick_ = tick_;
        corpse_resolved_ = false;
        respawn_ticks_remaining_ = kPlayerRespawnTicks;
    }
    return !alive();
}

void PlayerState::tick_corpse() {
    begin_tick();
    ++tick_;

    if (respawn_ticks_remaining_ > 0) {
        --respawn_ticks_remaining_;
    }

    if (death_phase_ == PlayerDeathPhase::Die1 &&
        tick_ - death_started_tick_ >= kPlayerDie1Ticks) {
        death_phase_ = PlayerDeathPhase::Die2;
    }
}

void PlayerState::mark_corpse_resolved() {
    corpse_resolved_ = true;
}

void PlayerState::reset_death_state() {
    pain_ticks_ = 0;
    punch_ticks_ = 0;
    punch_aim_up_ = false;
    punch_aim_down_ = false;
    death_phase_ = PlayerDeathPhase::None;
    death_started_tick_ = 0;
    death_health_ = 0;
    corpse_resolved_ = false;
    respawn_ticks_remaining_ = 0;
}

void PlayerState::start_punch(bool aim_up, bool aim_down) {
    if (!alive()) {
        return;
    }
    punch_ticks_ = kPlayerPunchFrames;
    punch_aim_up_ = aim_up;
    punch_aim_down_ = aim_down;
}

void PlayerState::restore_health() {
    health_ = kMaxHealth;
}

void PlayerState::set_armor(int value) {
    armor_ = std::clamp(value, 0, kArmorLimit);
}

bool PlayerState::give_key_red() {
    if (key_red_) {
        return false;
    }
    key_red_ = true;
    return true;
}

bool PlayerState::give_key_green() {
    if (key_green_) {
        return false;
    }
    key_green_ = true;
    return true;
}

bool PlayerState::give_key_blue() {
    if (key_blue_) {
        return false;
    }
    key_blue_ = true;
    return true;
}

void PlayerState::give_backpack() {
    has_backpack_ = true;
}

bool PlayerState::add_health(int amount, int max_health) {
    if (health_ >= max_health || amount <= 0) {
        return false;
    }
    health_ = std::min(health_ + amount, max_health);
    return true;
}

void PlayerState::set_health(int value) {
    health_ = std::max(0, value);
}

void PlayerState::update_movement_inputs(bool left, bool right) {
    if (left && !right) {
        run_direction_ = -1;
    } else if (right && !left) {
        run_direction_ = 1;
    } else if (!left && !right) {
        run_direction_ = 0;
    }

    if (run_direction_ == 1 && left) {
        combat_.facing_left = true;
    } else if (run_direction_ == -1 && right) {
        combat_.facing_left = false;
    } else if (run_direction_ == -1) {
        combat_.facing_left = true;
    } else if (run_direction_ == 1) {
        combat_.facing_left = false;
    }
}

void PlayerState::begin_tick() {
    prev_x = x;
    prev_y = y;
}

int PlayerState::z_dec(int value, int amount) {
    if (value > 0) {
        return std::max(0, value - amount);
    }
    if (value < 0) {
        return std::min(0, value + amount);
    }
    return 0;
}

void PlayerState::apply_run() {
    if (run_direction_ == -1 && vel_x > -kMaxRunVel) {
        vel_x -= (kMaxRunVel >> 3);
    } else if (run_direction_ == 1 && vel_x < kMaxRunVel) {
        vel_x += (kMaxRunVel >> 3);
    }
}

void PlayerState::try_jump(const MapCollision& collision) {
    if (jetpack_active_) {
        if (jet_fuel_ > 0) {
            if (vel_y > -kVelFly) {
                vel_y -= 3;
            }
            --jet_fuel_;
        }
        if (jet_fuel_ <= 0) {
            jetpack_active_ = false;
        }
        return;
    }

    const bool grounded =
        on_ground_ ||
        collision.on_ground(x, y, width, height) ||
        collision.collides_panel(x, y + 36.0f, width, height - 33.0f, map::PANEL_STEP);

    if (grounded && accel_y == 0) {
        vel_y = -kVelJump;
        on_ground_ = false;
        can_jetpack_ = false;
        return;
    }

    if (in_water_) {
        vel_y = -kVelSwim;
        return;
    }

    if (jet_fuel_ > 0 && can_jetpack_ && !in_water_) {
        jetpack_active_ = true;
        can_jetpack_ = false;
        if (vel_y > -kVelFly) {
            vel_y -= 3;
        }
        --jet_fuel_;
    }
}

void PlayerState::fixed_update(const MapCollision& collision, PlayerInput input) {
    begin_tick();

    const bool physics_tick = (tick_ % 2) == 0;
    ++tick_;
    on_lift_ = false;

    if (pain_ticks_ > 0) {
        --pain_ticks_;
    }

    if (punch_ticks_ > 0) {
        --punch_ticks_;
    }

    if (physics_tick) {
        update_movement_inputs(input.left, input.right);
        apply_run();

        switch (collision.vertical_lift_at(x, y, width, height)) {
        case -1:
            vel_y -= 1;
            if (vel_y < -5) {
                vel_y += 1;
            }
            on_lift_ = true;
            break;
        case 1:
            if (vel_y > 5) {
                vel_y -= 1;
            }
            vel_y += 1;
            on_lift_ = true;
            break;
        default:
            vel_y += 1;
            if (vel_y > kMaxYVel) {
                vel_y -= 1;
            }
            break;
        }

        switch (collision.horizontal_lift_at(x, y, width, height)) {
        case -1:
            vel_x -= 3;
            if (vel_x < -9) {
                vel_x += 3;
            }
            on_lift_ = true;
            break;
        case 1:
            vel_x += 3;
            if (vel_x > 9) {
                vel_x -= 3;
            }
            on_lift_ = true;
            break;
        default:
            break;
        }

        if (in_water_) {
            int damp_x = abs_int(vel_x) + 1;
            if (damp_x > 5) {
                vel_x = z_dec(vel_x, (damp_x / 2) - 2);
            }
            int damp_y = abs_int(vel_y) + 1;
            if (damp_y > 5) {
                vel_y = z_dec(vel_y, (damp_y / 2) - 2);
            }
        }

        accel_x = z_dec(accel_x, 1);
        accel_y = z_dec(accel_y, 1);

        if (input.jump) {
            try_jump(collision);
        }
    } else if (vel_x == 0) {
        update_movement_inputs(input.left, input.right);
        apply_run();
    }

    const int dx = vel_x + accel_x;
    const int dy = vel_y + accel_y;
    const std::uint16_t move_state =
        collision.move_object(x, y, width, height, dx, dy, true);

    in_water_ = (move_state & MOVE_INWATER) != 0;
    in_acid_ = collision.in_acid(x, y, width, height);
    on_ground_ = collision.on_ground(x, y, width, height);
    if (!on_lift_) {
        on_lift_ = collision.vertical_lift_at(x, y, width, height) != 0 ||
                   collision.horizontal_lift_at(x, y, width, height) != 0;
    }

    if (physics_tick) {
        if ((move_state & MOVE_HITWALL) != 0) {
            vel_x = 0;
            accel_x = 0;
        }
        if ((move_state & (MOVE_HITCEIL | MOVE_HITLAND)) != 0) {
            vel_y = 0;
            accel_y = 0;
        }
    }

    // Legacy: air resistance when not holding run keys (g_player.pas).
    if (!input.left && !input.right && vel_x != 0) {
        vel_x = z_dec(vel_x, 1);
    }

    if (!physics_tick) {
        update_movement_inputs(input.left, input.right);
    }

    if (!input.jump) {
        if (jetpack_active_) {
            jetpack_active_ = false;
        }
        can_jetpack_ = true;
    }
}

float PlayerState::render_x(float alpha) const {
    return prev_x + (x - prev_x) * alpha;
}

float PlayerState::render_y(float alpha) const {
    return prev_y + (y - prev_y) * alpha;
}

void PlayerState::restore_runtime_snapshot(const PlayerStateSnapshot& snap) {
    tick_ = snap.tick;
    powerup_suit_until_ = snap.powerup_suit_until;
    powerup_invul_until_ = snap.powerup_invul_until;
    powerup_invis_until_ = snap.powerup_invis_until;
    berserk_until_ = snap.berserk_until;
    jet_fuel_ = snap.jet_fuel;
    air_ = snap.air;
    key_red_ = snap.key_red;
    key_green_ = snap.key_green;
    key_blue_ = snap.key_blue;
    has_backpack_ = snap.has_backpack;
    jetpack_active_ = snap.jetpack_active;
    can_jetpack_ = snap.can_jetpack;
    on_ground_ = snap.on_ground;
    in_water_ = snap.in_water;
    in_acid_ = snap.in_acid;
    on_lift_ = snap.on_lift;
    run_direction_ = snap.run_direction;
    pain_ticks_ = snap.pain_ticks;
    punch_ticks_ = snap.punch_ticks;
    punch_aim_up_ = snap.punch_aim_up;
    punch_aim_down_ = snap.punch_aim_down;
    death_phase_ = snap.death_phase;
    death_started_tick_ = snap.death_started_tick;
    death_health_ = snap.death_health;
    corpse_resolved_ = snap.corpse_resolved;
    respawn_ticks_remaining_ = snap.respawn_ticks_remaining;
}

} // namespace d2df::sim
