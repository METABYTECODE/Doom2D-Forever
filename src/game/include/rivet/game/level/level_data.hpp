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

struct LevelData {
    static constexpr const char* kFormatId = "rivet-level";
    static constexpr int kVersion = 1;

    std::string name;
    int tile_size = 32;
    int width = 0;
    int height = 0;
    std::vector<std::vector<int>> tiles;
    std::vector<LevelObject> objects;
};

} // namespace rivet::game::level
