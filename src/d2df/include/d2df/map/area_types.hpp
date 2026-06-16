#pragma once

#include <cstdint>
#include <string_view>

namespace d2df::map {

enum class AreaType : std::uint8_t {
    None = 0,
    PlayerPoint1 = 1,
    PlayerPoint2 = 2,
    DmPoint = 3,
    RedFlag = 4,
    BlueFlag = 5,
    DomFlag = 6,
    RedTeamPoint = 7,
    BlueTeamPoint = 8,
};

[[nodiscard]] AreaType area_type_from_name(std::string_view name);

} // namespace d2df::map
