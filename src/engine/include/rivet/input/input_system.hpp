#pragma once

#include <array>
#include <cstddef>

#include <rivet/input/keys.hpp>

namespace rivet::input {

struct InputState {
    std::array<bool, static_cast<std::size_t>(Key::Count)> keys{};
    bool quit_requested = false;
    float move_x = 0.0f;
    float move_y = 0.0f;

    float mouse_x = 0.0f;
    float mouse_y = 0.0f;
    float mouse_wheel_y = 0.0f;
    bool mouse_left_down = false;
    bool mouse_right_down = false;
    bool mouse_middle_down = false;
};

/// Platform layer feeds polled input here; gameplay reads it via game layer later.
class InputSystem {
public:
    void begin_frame();
    void end_frame();
    void update_axes();

    [[nodiscard]] bool is_key_down(Key key) const;
    [[nodiscard]] bool is_key_pressed(Key key) const;
    void set_key_down(Key key, bool down);

    [[nodiscard]] bool is_mouse_down(int button) const;
    [[nodiscard]] bool is_mouse_pressed(int button) const;
    void set_mouse_position(float x, float y);
    void set_mouse_button(int button, bool down);
    void add_mouse_wheel(float delta_y);

    [[nodiscard]] const InputState& state() const { return state_; }
    InputState& state() { return state_; }

private:
    InputState state_;
    InputState previous_;
};

} // namespace rivet::input
