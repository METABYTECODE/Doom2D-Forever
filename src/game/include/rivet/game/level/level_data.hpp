#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace rivet::game::level {

/// Native Rivet level document. Editor and runtime share this schema.
struct LevelObject {
    std::string id;
    std::string type;
    float x = 0.0f;
    float y = 0.0f;
    float width = 48.0f;
    float height = 48.0f;
    float vel_x = 0.0f;
    float vel_y = 0.0f;
};

struct TileFrame {
    std::string tileset;
    int id = 0;
};

/// Graphics tile instance. Origin is in level grid cells (see grid_size).
struct PlacedTile {
    std::string tileset;
    int id = 0;
    int x = 0;
    int y = 0;
    std::vector<TileFrame> frames;
    int frame_ms = 120;
};

[[nodiscard]] inline bool is_animated_tile(const PlacedTile& tile) {
    return tile.frames.size() > 1;
}

[[nodiscard]] inline TileFrame tile_frame_at_time(const PlacedTile& tile, const float time_seconds) {
    if (tile.frames.size() <= 1) {
        if (tile.frames.empty()) {
            return TileFrame{.tileset = tile.tileset, .id = tile.id};
        }
        return tile.frames.front();
    }

    const int frame_ms = tile.frame_ms > 0 ? tile.frame_ms : 120;
    const auto elapsed_ms = static_cast<std::int64_t>(time_seconds * 1000.0f);
    const auto index = static_cast<std::size_t>(
        (elapsed_ms / frame_ms) % static_cast<std::int64_t>(tile.frames.size()));
    return tile.frames[index];
}

struct LevelData {
    static constexpr const char* kFormatId = "rivet-level";
    static constexpr int kVersion = 2;

    std::string name;
    int grid_size = 8;
    int width = 0;
    int height = 0;
    std::string background;
    std::string music;
    std::vector<PlacedTile> tiles;
    std::vector<std::vector<int>> collision;
    std::vector<LevelObject> objects;
};

} // namespace rivet::game::level
