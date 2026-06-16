#include <d2df/map/map_spawn.hpp>

#include <d2df/map/area_types.hpp>

namespace d2df::map {
namespace {

const MapArea* first_area(const MapDocument& map, AreaType type) {
    for (const auto& area : map.areas) {
        if (area.type == type) {
            return &area;
        }
    }
    return nullptr;
}

std::size_t count_areas(const MapDocument& map, AreaType type) {
    std::size_t count = 0;
    for (const auto& area : map.areas) {
        if (area.type == type) {
            ++count;
        }
    }
    return count;
}

} // namespace

std::optional<MapPoint> find_player_spawn(const MapDocument& map, int player_slot) {
    AreaType primary = player_slot == 0 ? AreaType::PlayerPoint1 : AreaType::PlayerPoint2;
    AreaType fallback = player_slot == 0 ? AreaType::PlayerPoint2 : AreaType::PlayerPoint1;

    if (const MapArea* area = first_area(map, primary)) {
        return area->position;
    }
    if (const MapArea* area = first_area(map, fallback)) {
        return area->position;
    }
    if (count_areas(map, AreaType::DmPoint) > 0) {
        return first_area(map, AreaType::DmPoint)->position;
    }
    if (map.size.width > 0 && map.size.height > 0) {
        return MapPoint{map.size.width / 2, map.size.height / 2};
    }
    return std::nullopt;
}

} // namespace d2df::map
