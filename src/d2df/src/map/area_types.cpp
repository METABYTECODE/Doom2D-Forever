#include <d2df/map/area_types.hpp>

namespace d2df::map {

AreaType area_type_from_name(std::string_view name) {
    if (name == "AREA_PLAYERPOINT1") {
        return AreaType::PlayerPoint1;
    }
    if (name == "AREA_PLAYERPOINT2") {
        return AreaType::PlayerPoint2;
    }
    if (name == "AREA_DMPOINT") {
        return AreaType::DmPoint;
    }
    if (name == "AREA_REDFLAG") {
        return AreaType::RedFlag;
    }
    if (name == "AREA_BLUEFLAG") {
        return AreaType::BlueFlag;
    }
    if (name == "AREA_DOMFLAG") {
        return AreaType::DomFlag;
    }
    if (name == "AREA_REDTEAMPOINT") {
        return AreaType::RedTeamPoint;
    }
    if (name == "AREA_BLUETEAMPOINT") {
        return AreaType::BlueTeamPoint;
    }
    return AreaType::None;
}

} // namespace d2df::map
