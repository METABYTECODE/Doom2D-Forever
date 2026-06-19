#include <algorithm>
#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include <rivet/game/level/level_saver.hpp>
#include <rivet/game/resources/resource_pack.hpp>

namespace rivet::game::level {

namespace {

[[nodiscard]] bool fluids_grid_has_content(const std::vector<std::vector<int>>& fluids) {
    for (const auto& row : fluids) {
        for (const int cell : row) {
            if (cell != kFluidNone) {
                return true;
            }
        }
    }
    return false;
}

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
    nlohmann::json json = {
        {"tileset", tile.tileset},
        {"id", tile.id},
        {"x", tile.x},
        {"y", tile.y},
    };
    if (tile.frames.size() > 1) {
        nlohmann::json frames = nlohmann::json::array();
        for (const auto& frame : tile.frames) {
            frames.push_back({{"tileset", frame.tileset}, {"id", frame.id}});
        }
        json["frames"] = std::move(frames);
        json["frame_ms"] = tile.frame_ms;
    }
    return json;
}

[[nodiscard]] bool fluids_grid_matches(const LevelData& level) {
    const int cols = std::max(1, (level.width + level.grid_size - 1) / level.grid_size);
    const int rows = std::max(1, (level.height + level.grid_size - 1) / level.grid_size);
    if (static_cast<int>(level.fluids.size()) != rows) {
        return false;
    }
    for (const auto& row : level.fluids) {
        if (static_cast<int>(row.size()) != cols) {
            return false;
        }
    }
    return true;
}

} // namespace

void save_level(const std::filesystem::path& path, const LevelData& level) {
    if (level.width <= 0 || level.height <= 0) {
        throw std::runtime_error("Cannot save level with non-positive dimensions");
    }
    const int cols = std::max(1, (level.width + level.grid_size - 1) / level.grid_size);
    const int rows = std::max(1, (level.height + level.grid_size - 1) / level.grid_size);
    if (static_cast<int>(level.collision.size()) != rows) {
        throw std::runtime_error("Level collision row count does not match sub-grid height");
    }
    for (const auto& row : level.collision) {
        if (static_cast<int>(row.size()) != cols) {
            throw std::runtime_error("Level collision row width does not match sub-grid width");
        }
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

    if (fluids_grid_matches(level) && fluids_grid_has_content(level.fluids)) {
        json["fluids"] = level.fluids;
    }

    if (!level.background.empty()) {
        json["background"] = level.background;
    }
    if (!level.music.empty()) {
        json["music"] = level.music;
    }
    if (level.resource_pack != resources::kDefaultPackId) {
        json["resource_pack"] = level.resource_pack;
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
