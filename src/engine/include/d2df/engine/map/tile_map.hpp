#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace d2df::engine::map {

struct TileDef {
    std::string asset_id;
    std::int32_t width = 0;
    std::int32_t height = 0;
    bool animated = false;
};

struct TilePlacement {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::uint16_t tile_index = 0;
    std::uint32_t flags = 0;
    std::uint8_t visibility = 0;
    std::uint8_t alpha = 0;
    bool enabled = true;
};

struct TileLayer {
    std::string name;
    std::vector<TilePlacement> tiles;
};

struct SpawnPoint {
    std::string id;
    std::int32_t x = 0;
    std::int32_t y = 0;
};

struct MapEntity {
    std::string type;
    std::int32_t x = 0;
    std::int32_t y = 0;
    float width = 0.0f;
    float height = 0.0f;
};

/// Native engine map: ordered tile layers + spawn points + placed entities.
struct TileMap {
    static constexpr const char* kFormatId = "d2df-tilemap";

    int version = 1;
    std::string id;
    std::string name;
    std::string author;
    std::string description;
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::string sky_asset;
    std::string music_asset;
    std::vector<TileDef> tileset;
    std::vector<TileLayer> layers;
    std::vector<SpawnPoint> spawns;
    std::vector<MapEntity> entities;
};

} // namespace d2df::engine::map
