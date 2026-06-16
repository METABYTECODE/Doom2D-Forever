#include <d2df/sim/player_state.hpp>

#include <algorithm>
#include <cmath>

#include <d2df/map/panel_types.hpp>
#include <d2df/sim/map_collision.hpp>

namespace d2df::sim {
namespace {

int abs_int(int value) {
    return value < 0 ? -value : value;
}

} // namespace

void PlayerState::snap_to(float spawn_x, float spawn_y) {
    x = spawn_x;
    y = spawn_y;
    prev_x = spawn_x;
    prev_y = spawn_y;
    vel_x = 0;
    vel_y = 0;
    accel_x = 0;
    accel_y = 0;
    tick_ = 0;
    on_ground_ = false;
    in_water_ = false;
    in_acid_ = false;
    on_lift_ = false;
    health_ = kMaxHealth;
    combat_.reset_single_player_loadout();
    armor_ = 0;
    key_red_ = false;
    key_green_ = false;
    key_blue_ = false;
}

bool PlayerState::apply_damage(int amount) {
    if (amount <= 0 || !alive()) {
        return false;
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

    health_ = std::max(0, health_ - remaining);
    return !alive();
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

void PlayerState::update_facing(bool left, bool right) {
    if (left && !right) {
        combat_.facing_left = true;
    } else if (right && !left) {
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

void PlayerState::apply_run(bool left, bool right) {
    if (left && vel_x > -kMaxRunVel) {
        vel_x -= (kMaxRunVel >> 3);
    }
    if (right && vel_x < kMaxRunVel) {
        vel_x += (kMaxRunVel >> 3);
    }
}

void PlayerState::try_jump(const MapCollision& collision) {
    const bool grounded =
        on_ground_ ||
        collision.on_ground(x, y, width, height) ||
        collision.collides_panel(x, y + 36.0f, width, height - 33.0f, map::PANEL_STEP);

    if (grounded && accel_y == 0) {
        vel_y = -kVelJump;
        on_ground_ = false;
        return;
    }

    if (in_water_) {
        vel_y = -kVelSwim;
    }
}

void PlayerState::fixed_update(const MapCollision& collision, PlayerInput input) {
    begin_tick();

    const bool physics_tick = (tick_ % 2) == 0;
    ++tick_;
    on_lift_ = false;

    if (physics_tick) {
        apply_run(input.left, input.right);

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
        apply_run(input.left, input.right);
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

    update_facing(input.left, input.right);
}

float PlayerState::render_x(float alpha) const {
    return prev_x + (x - prev_x) * alpha;
}

float PlayerState::render_y(float alpha) const {
    return prev_y + (y - prev_y) * alpha;
}

} // namespace d2df::sim
