#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace d2df::tools {

struct MapPoint {
    std::int32_t x = 0;
    std::int32_t y = 0;
};

struct MapSize {
    std::int32_t width = 0;
    std::int32_t height = 0;
};

struct LegacyMapHeader {
    std::string name;
    std::string author;
    std::string description;
    std::string music;
    std::string sky;
    MapSize size;
};

struct LegacyTexture {
    std::string path;
    bool animated = false;
};

struct LegacyPanel {
    MapPoint position;
    MapSize size;
    std::uint16_t texture = 0;
    std::uint16_t type = 0;
    std::uint8_t alpha = 0;
    std::uint8_t flags = 0;
};

struct LegacyItem {
    MapPoint position;
    std::uint8_t type = 0;
    std::uint8_t options = 0;
};

struct LegacyMonster {
    MapPoint position;
    std::uint8_t type = 0;
    std::uint8_t direction = 0;
};

struct LegacyArea {
    MapPoint position;
    std::uint8_t type = 0;
    std::uint8_t direction = 0;
};

struct LegacyTrigger {
    MapPoint position;
    MapSize size;
    bool enabled = true;
    std::int32_t texture_panel = -1;
    std::uint8_t type = 0;
    std::uint8_t activate_type = 0;
    std::uint8_t keys = 0;
    std::array<std::uint8_t, 128> triggerdata{};
};

struct LegacyMapDocument {
    LegacyMapHeader header;
    std::vector<LegacyTexture> textures;
    std::vector<LegacyPanel> panels;
    std::vector<LegacyItem> items;
    std::vector<LegacyArea> areas;
    std::vector<LegacyMonster> monsters;
    std::vector<LegacyTrigger> triggers;
};

[[nodiscard]] LegacyMapDocument parse_legacy_mapbin(std::span<const std::uint8_t> data);

[[nodiscard]] std::string read_legacy_string(std::span<const std::uint8_t> data, std::size_t offset,
                                             std::size_t max_len);

} // namespace d2df::tools
