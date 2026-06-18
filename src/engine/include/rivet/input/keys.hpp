#pragma once

#include <cstdint>

namespace rivet::input {

enum class Key : std::uint16_t {
    Unknown = 0,
    Escape,
    Left,
    Right,
    Up,
    Down,
    W,
    A,
    S,
    D,
    Space,
    Count,
};

} // namespace rivet::input
