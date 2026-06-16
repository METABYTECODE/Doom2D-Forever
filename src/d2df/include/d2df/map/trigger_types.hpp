#pragma once

#include <cstdint>
#include <string_view>

namespace d2df::map {

enum class TriggerType : std::uint8_t {
    None = 0,
    Exit = 1,
    Teleport = 2,
    OpenDoor = 3,
    CloseDoor = 4,
    Door = 5,
    Door5 = 6,
    CloseTrap = 7,
    Trap = 8,
    Press = 9,
    Secret = 10,
    LiftUp = 11,
    LiftDown = 12,
    Lift = 13,
    On = 15,
    Off = 16,
    OnOff = 17,
};

enum class ActivateType : std::uint8_t {
    None = 0,
    PlayerCollide = 1 << 0,
    MonsterCollide = 1 << 1,
    PlayerPress = 1 << 2,
    MonsterPress = 1 << 3,
    Shot = 1 << 4,
    NoMonster = 1 << 5,
};

[[nodiscard]] TriggerType trigger_type_from_name(std::string_view name);
[[nodiscard]] ActivateType activate_type_from_json(std::string_view activate_array_snippet);
[[nodiscard]] bool has_activate_on_load(ActivateType flags);

} // namespace d2df::map
