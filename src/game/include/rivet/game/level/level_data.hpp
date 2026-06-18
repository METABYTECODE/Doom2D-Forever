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

/// Graphics tile instance. Origin is in level grid cells (see grid_size).
struct PlacedTile {
    std::string tileset;
    int id = 0;
    int x = 0;
    int y = 0;
};

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
