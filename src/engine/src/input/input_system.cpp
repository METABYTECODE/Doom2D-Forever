#include <rivet/input/input_system.hpp>

namespace rivet::input {

namespace {

[[nodiscard]] std::size_t key_index(const Key key) {
    return static_cast<std::size_t>(key);
}

} // namespace

void InputSystem::begin_frame() {
    previous_ = state_;
    state_.quit_requested = false;
    state_.move_x = 0.0f;
    state_.move_y = 0.0f;
    state_.mouse_wheel_y = 0.0f;
}

void InputSystem::end_frame() {}

void InputSystem::update_axes() {
    auto axis = [&](Key negative, Key positive) -> float {
        float value = 0.0f;
        if (is_key_down(negative)) {
            value -= 1.0f;
        }
        if (is_key_down(positive)) {
            value += 1.0f;
        }
        return value;
    };

    state_.move_x = axis(Key::A, Key::D);
    if (state_.move_x == 0.0f) {
        state_.move_x = axis(Key::Left, Key::Right);
    }

    state_.move_y = axis(Key::W, Key::S);
    if (state_.move_y == 0.0f) {
        state_.move_y = axis(Key::Up, Key::Down);
    }
}

bool InputSystem::is_key_down(const Key key) const {
    const auto index = key_index(key);
    if (index >= state_.keys.size()) {
        return false;
    }
    return state_.keys[index];
}

bool InputSystem::is_key_pressed(const Key key) const {
    const auto index = key_index(key);
    if (index >= state_.keys.size()) {
        return false;
    }
    return state_.keys[index] && !previous_.keys[index];
}

void InputSystem::set_key_down(const Key key, const bool down) {
    const auto index = key_index(key);
    if (index < state_.keys.size()) {
        state_.keys[index] = down;
    }
}

bool InputSystem::is_mouse_down(const int button) const {
    switch (button) {
    case 1:
        return state_.mouse_left_down;
    case 2:
        return state_.mouse_middle_down;
    case 3:
        return state_.mouse_right_down;
    default:
        return false;
    }
}

bool InputSystem::is_mouse_pressed(const int button) const {
    switch (button) {
    case 1:
        return state_.mouse_left_down && !previous_.mouse_left_down;
    case 2:
        return state_.mouse_middle_down && !previous_.mouse_middle_down;
    case 3:
        return state_.mouse_right_down && !previous_.mouse_right_down;
    default:
        return false;
    }
}

void InputSystem::set_mouse_position(const float x, const float y) {
    state_.mouse_x = x;
    state_.mouse_y = y;
}

void InputSystem::set_mouse_button(const int button, const bool down) {
    switch (button) {
    case 1:
        state_.mouse_left_down = down;
        break;
    case 2:
        state_.mouse_middle_down = down;
        break;
    case 3:
        state_.mouse_right_down = down;
        break;
    default:
        break;
    }
}

void InputSystem::add_mouse_wheel(const float delta_y) {
    state_.mouse_wheel_y += delta_y;
}

} // namespace rivet::input
