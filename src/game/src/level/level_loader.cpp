#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include <rivet/game/level/level_loader.hpp>

namespace rivet::game::level {

namespace {

[[nodiscard]] LevelObject parse_object(const nlohmann::json& json) {
    LevelObject object;
    object.id = json.value("id", "");
    object.type = json.at("type").get<std::string>();
    object.x = json.at("x").get<float>();
    object.y = json.at("y").get<float>();

    if (json.contains("width")) {
        object.width = json.at("width").get<float>();
    }
    if (json.contains("height")) {
        object.height = json.at("height").get<float>();
    }
    if (json.contains("vx")) {
        object.vel_x = json.at("vx").get<float>();
    }
    if (json.contains("vy")) {
        object.vel_y = json.at("vy").get<float>();
    }
    return object;
}

} // namespace

LevelData load_level(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Failed to open level file: " + path.string());
    }

    const auto json = nlohmann::json::parse(input);
    if (json.at("format").get<std::string>() != LevelData::kFormatId) {
        throw std::runtime_error("Unsupported level format in: " + path.string());
    }
    if (json.at("version").get<int>() != LevelData::kVersion) {
        throw std::runtime_error("Unsupported level version in: " + path.string());
    }

    LevelData level;
    level.name = json.value("name", path.stem().string());
    level.tile_size = json.value("tile_size", 32);
    level.width = json.value("width", 0);
    level.height = json.value("height", 0);
    level.tiles = json.at("tiles").get<std::vector<std::vector<int>>>();

    if (level.height == 0) {
        level.height = static_cast<int>(level.tiles.size());
    }
    if (level.width == 0 && !level.tiles.empty()) {
        level.width = static_cast<int>(level.tiles.front().size());
    }

    for (const auto& row : level.tiles) {
        if (static_cast<int>(row.size()) != level.width) {
            throw std::runtime_error("Level tile rows have inconsistent width: " + path.string());
        }
    }

    if (json.contains("objects")) {
        for (const auto& object_json : json.at("objects")) {
            level.objects.push_back(parse_object(object_json));
        }
    }

    return level;
}

} // namespace rivet::game::level
