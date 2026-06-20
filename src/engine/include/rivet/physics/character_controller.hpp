#pragma once

namespace rivet::physics {

/// Kinematic character movement: Input → forces → velocity (HK-style layers).
/// Collision resolution stays in PhysicsWorld::step.
class CharacterController {
public:
    struct Config {
        float gravity = 2200.0f;
        float water_factor = 0.4f;
        float max_fall_speed = 900.0f;
        float max_run_speed = 220.0f;
        float grounded_accel = 6000.0f;
        float air_accel = 3500.0f;
        float grounded_decel = 8000.0f;
        float air_decel = 2000.0f;
        float jump_speed = 660.0f;
        float water_drag_x = 4.0f;
        float water_drag_y = 1.8f;
        float swim_accel = 3200.0f;
        float max_water_vx = 160.0f;
        float max_water_vy = 240.0f;
        float immersion_threshold = 0.01f;
        float move_deadzone = 0.05f;
    };

    struct Input {
        float move_x = 0.0f;
        bool jump_held = false;
    };

    struct Environment {
        bool in_water = false;
        bool grounded = false;
    };

    explicit CharacterController(Config config = {});

    [[nodiscard]] const Config& config() const { return config_; }
    void set_config(const Config& config) { config_ = config; }

    void simulate(
        float& velocity_x,
        float& velocity_y,
        const Input& input,
        const Environment& environment,
        float fixed_delta_time) const;

private:
    Config config_;
};

} // namespace rivet::physics
