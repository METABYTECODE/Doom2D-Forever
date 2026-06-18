#pragma once

#include <cstdint>

namespace d2df::engine::map {

/// Collision and render classification for placed tiles.
/// Low bits align with legacy `d2df::map::PANEL_*` for adapter compatibility.
inline constexpr std::uint32_t TILE_WALL = 1;
inline constexpr std::uint32_t TILE_BACK = 2;
inline constexpr std::uint32_t TILE_FORE = 4;
inline constexpr std::uint32_t TILE_WATER = 8;
inline constexpr std::uint32_t TILE_ACID1 = 16;
inline constexpr std::uint32_t TILE_ACID2 = 32;
inline constexpr std::uint32_t TILE_STEP = 64;
inline constexpr std::uint32_t TILE_LIFT_UP = 128;
inline constexpr std::uint32_t TILE_LIFT_DOWN = 256;
inline constexpr std::uint32_t TILE_OPEN_DOOR = 512;
inline constexpr std::uint32_t TILE_CLOSE_DOOR = 1024;
inline constexpr std::uint32_t TILE_BLOCK_MON = 2048;
inline constexpr std::uint32_t TILE_LIFT_LEFT = 4096;
inline constexpr std::uint32_t TILE_LIFT_RIGHT = 8192;

inline constexpr std::uint8_t TILE_VIS_HIDDEN = 2;
inline constexpr std::uint8_t TILE_VIS_BLEND = 1;

[[nodiscard]] inline bool tile_is_solid(std::uint32_t flags, bool enabled) {
    if ((flags & TILE_WALL) != 0) {
        return true;
    }
    if ((flags & TILE_CLOSE_DOOR) != 0) {
        return enabled;
    }
    if ((flags & TILE_OPEN_DOOR) != 0) {
        return enabled;
    }
    return false;
}

[[nodiscard]] inline bool tile_participates_in_collision(std::uint32_t flags, std::uint8_t visibility,
                                                         bool enabled) {
    if ((visibility & TILE_VIS_HIDDEN) == 0) {
        return true;
    }
    if ((flags & (TILE_CLOSE_DOOR | TILE_OPEN_DOOR)) != 0) {
        return true;
    }
    (void)enabled;
    return false;
}

} // namespace d2df::engine::map
