#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include <rivet/game/level/level_saver.hpp>

namespace rivet::game::level {

namespace {

[[nodiscard]] nlohmann::json object_to_json(const LevelObject& object) {
    nlohmann::json json = {
        {"type", object.type},
        {"x", object.x},
        {"y", object.y},
    };

    if (!object.id.empty()) {
        json["id"] = object.id;
    }
    if (object.width != 48.0f) {
        json["width"] = object.width;
    }
    if (object.height != 48.0f) {
        json["height"] = object.height;
    }
    if (object.vel_x != 0.0f) {
        json["vx"] = object.vel_x;
    }
    if (object.vel_y != 0.0f) {
        json["vy"] = object.vel_y;
    }
    return json;
}

[[nodiscard]] nlohmann::json placed_tile_to_json(const PlacedTile& tile) {
    return {
        {"tileset", tile.tileset},
        {"id", tile.id},
        {"x", tile.x},
        {"y", tile.y},
    };
}

} // namespace

void save_level(const std::filesystem::path& path, const LevelData& level) {
    if (level.width <= 0 || level.height <= 0) {
        throw std::runtime_error("Cannot save level with non-positive dimensions");
    }
    if (static_cast<int>(level.collision.size()) != level.height) {
        throw std::runtime_error("Level collision row count does not match height");
    }

    nlohmann::json json = {
        {"format", LevelData::kFormatId},
        {"version", LevelData::kVersion},
        {"name", level.name},
        {"grid_size", level.grid_size},
        {"width", level.width},
        {"height", level.height},
        {"tiles", nlohmann::json::array()},
        {"collision", level.collision},
        {"objects", nlohmann::json::array()},
    };

    if (!level.background.empty()) {
        json["background"] = level.background;
    }
    if (!level.music.empty()) {
        json["music"] = level.music;
    }

    for (const auto& tile : level.tiles) {
        json["tiles"].push_back(placed_tile_to_json(tile));
    }

    for (const auto& object : level.objects) {
        json["objects"].push_back(object_to_json(object));
    }

    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Failed to open level file for writing: " + path.string());
    }
    output << json.dump(2) << '\n';
}

} // namespace rivet::game::level
