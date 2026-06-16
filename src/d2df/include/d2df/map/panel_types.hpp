#pragma once

#include <cstdint>
#include <string_view>

namespace d2df::map {

inline constexpr std::uint16_t PANEL_WALL = 1;
inline constexpr std::uint16_t PANEL_BACK = 2;
inline constexpr std::uint16_t PANEL_FORE = 4;
inline constexpr std::uint16_t PANEL_WATER = 8;
inline constexpr std::uint16_t PANEL_ACID1 = 16;
inline constexpr std::uint16_t PANEL_ACID2 = 32;
inline constexpr std::uint16_t PANEL_STEP = 64;
inline constexpr std::uint16_t PANEL_LIFTUP = 128;
inline constexpr std::uint16_t PANEL_LIFTDOWN = 256;
inline constexpr std::uint16_t PANEL_OPENDOOR = 512;
inline constexpr std::uint16_t PANEL_CLOSEDOOR = 1024;
inline constexpr std::uint16_t PANEL_BLOCKMON = 2048;
inline constexpr std::uint16_t PANEL_LIFTLEFT = 4096;
inline constexpr std::uint16_t PANEL_LIFTRIGHT = 8192;

inline constexpr std::uint8_t PANEL_FLAG_HIDE = 2;
inline constexpr std::uint8_t PANEL_FLAG_BLENDING = 1;

[[nodiscard]] std::uint16_t panel_type_from_name(std::string_view name);

} // namespace d2df::map
