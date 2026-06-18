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

} // namespace

void save_level(const std::filesystem::path& path, const LevelData& level) {
    if (level.width <= 0 || level.height <= 0) {
        throw std::runtime_error("Cannot save level with non-positive dimensions");
    }
    if (static_cast<int>(level.tiles.size()) != level.height) {
        throw std::runtime_error("Level tile row count does not match height");
    }

    nlohmann::json json = {
        {"format", LevelData::kFormatId},
        {"version", LevelData::kVersion},
        {"name", level.name},
        {"tile_size", level.tile_size},
        {"width", level.width},
        {"height", level.height},
        {"tiles", level.tiles},
        {"objects", nlohmann::json::array()},
    };

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
