#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <d2df/map/area_types.hpp>
#include <d2df/map/trigger_types.hpp>

namespace d2df::map {

struct MapPoint {
    std::int32_t x = 0;
    std::int32_t y = 0;
};

struct MapSize {
    std::int32_t width = 0;
    std::int32_t height = 0;
};

struct MapTexture {
    std::string path;
    bool animated = false;
};

struct MapPanel {
    MapPoint position;
    MapSize size;
    std::uint16_t texture = 0;
    std::uint16_t type = 0;
    std::uint8_t alpha = 0;
    std::uint8_t flags = 0;
    bool enabled = true;
};

struct MapArea {
    MapPoint position;
    AreaType type = AreaType::None;
};

struct MapTrigger {
    MapPoint position;
    MapSize size;
    TriggerType type = TriggerType::None;
    bool enabled = true;
    std::int32_t target_panel = -1;
    ActivateType activate = ActivateType::None;
    MapPoint press_position;
    MapSize press_size;
    int press_wait = 0;
    int press_count_required = 0;
    bool ext_random = false;
    MapPoint teleport_target;
    bool d2d = false;
};

struct MapDocument {
    int version = 0;
    std::string id;
    std::string name;
    std::string author;
    std::string description;
    MapSize size;
    std::string music;
    std::string sky;
    std::vector<MapTexture> textures;
    std::vector<MapPanel> panels;
    std::vector<MapArea> areas;
    std::vector<MapTrigger> triggers;
};

} // namespace d2df::map
