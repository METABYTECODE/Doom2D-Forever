#pragma once

#include <string_view>

namespace rivet::game::model::anim {

/// Built-in animation names (rivet-model v2). Match Model Editor constants.
inline constexpr std::string_view Idle = "IDLE";
inline constexpr std::string_view Walk = "WALK";
inline constexpr std::string_view Run = "RUN";
inline constexpr std::string_view Fire = "FIRE";
inline constexpr std::string_view FireUp = "FIRE_UP";
inline constexpr std::string_view FireDown = "FIRE_DOWN";
inline constexpr std::string_view Pain = "PAIN";
inline constexpr std::string_view Die = "DIE";
inline constexpr std::string_view Jump = "JUMP";
inline constexpr std::string_view Fall = "FALL";

} // namespace rivet::game::model::anim
