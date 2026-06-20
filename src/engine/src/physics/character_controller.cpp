#include <rivet/physics/character_controller.hpp>

#include <algorithm>
#include <cmath>

namespace rivet::physics {

namespace {

[[nodiscard]] float sign_nonzero(const float value) {
    if (value > 0.0f) {
        return 1.0f;
    }
    if (value < 0.0f) {
        return -1.0f;
    }
    return 0.0f;
}

} // namespace

CharacterController::CharacterController(const Config config) : config_(config) {}

void CharacterController::simulate(
    float& velocity_x,
    float& velocity_y,
    const Input& input,
    const Environment& environment,
    const float fixed_delta_time) const {
    const float dt = fixed_delta_time;

    // 1. Horizontal input
    const bool moving = std::abs(input.move_x) >= config_.move_deadzone;
    if (moving) {
        const float target_vx = input.move_x * config_.max_run_speed;
        const float accel = environment.grounded ? config_.grounded_accel : config_.air_accel;
        const float diff = target_vx - velocity_x;
        const float max_step = accel * dt;
        velocity_x += std::clamp(diff, -max_step, max_step);
    } else {
        const float decel = environment.grounded ? config_.grounded_decel : config_.air_decel;
        const float step = decel * dt;
        if (std::abs(velocity_x) <= step) {
            velocity_x = 0.0f;
        } else {
            velocity_x -= sign_nonzero(velocity_x) * step;
        }
    }

    // 2. Gravity (single final value)
    float gravity = config_.gravity;
    if (environment.in_water) {
        gravity *= config_.water_factor;
    }
    velocity_y += gravity * dt;

    if (environment.in_water) {
        // 3. Swim input
        if (input.jump_held) {
            velocity_y -= config_.swim_accel * dt;
            velocity_y = std::max(velocity_y, -config_.max_water_vy);
        }

        // 4. Drag (after all forces)
        velocity_x *= std::exp(-config_.water_drag_x * dt);
        velocity_y *= std::exp(-config_.water_drag_y * dt);

        // 5. Clamps (end of physics)
        velocity_x = std::clamp(velocity_x, -config_.max_water_vx, config_.max_water_vx);
        velocity_y = std::min(velocity_y, config_.max_water_vy);
    } else if (velocity_y > config_.max_fall_speed) {
        velocity_y = config_.max_fall_speed;
    }
}

} // namespace rivet::physics
