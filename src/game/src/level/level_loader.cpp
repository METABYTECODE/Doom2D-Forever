#include <rivet/game/level/level_loader.hpp>

#include <algorithm>
#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include <rivet/game/resources/resource_pack.hpp>

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
    if (json.contains("z")) {
        object.z = json.at("z").get<int>();
    }
    if (json.contains("model")) {
        object.model = json.at("model").get<std::string>();
    }
    return object;
}

[[nodiscard]] TileFrame parse_tile_frame(const nlohmann::json& json) {
    return {
        .tileset = json.at("tileset").get<std::string>(),
        .id = json.at("id").get<int>(),
    };
}

[[nodiscard]] PlacedTile parse_placed_tile(const nlohmann::json& json) {
    PlacedTile tile;
    tile.tileset = json.at("tileset").get<std::string>();
    tile.id = json.at("id").get<int>();
    tile.x = json.at("x").get<int>();
    tile.y = json.at("y").get<int>();
    if (json.contains("frames")) {
        for (const auto& frame_json : json.at("frames")) {
            tile.frames.push_back(parse_tile_frame(frame_json));
        }
    }
    if (json.contains("frame_ms")) {
        tile.frame_ms = json.at("frame_ms").get<int>();
    }
    return tile;
}

[[nodiscard]] TileLayer parse_tile_layer(const nlohmann::json& json) {
    TileLayer layer;
    layer.id = json.value("id", "main");
    layer.z = json.value("z", 0);
    if (json.contains("tiles")) {
        for (const auto& tile_json : json.at("tiles")) {
            layer.tiles.push_back(parse_placed_tile(tile_json));
        }
    }
    return layer;
}

[[nodiscard]] std::vector<std::vector<int>> empty_collision_grid(const int width, const int height) {
    return std::vector<std::vector<int>>(
        static_cast<std::size_t>(height),
        std::vector<int>(static_cast<std::size_t>(width), 0));
}

[[nodiscard]] std::vector<std::vector<int>> empty_fluids_grid(const int width, const int height) {
    return empty_collision_grid(width, height);
}

[[nodiscard]] int sub_grid_cols(const int width_px, const int grid_size) {
    return std::max(1, (width_px + grid_size - 1) / grid_size);
}

[[nodiscard]] int sub_grid_rows(const int height_px, const int grid_size) {
    return std::max(1, (height_px + grid_size - 1) / grid_size);
}

void validate_collision_and_fluids(LevelData& level, const std::filesystem::path& path) {
    const int cols = sub_grid_cols(level.width, level.grid_size);
    const int rows = sub_grid_rows(level.height, level.grid_size);

    if (static_cast<int>(level.collision.size()) != rows) {
        throw std::runtime_error("Collision row count does not match level height: " + path.string());
    }
    for (const auto& row : level.collision) {
        if (static_cast<int>(row.size()) != cols) {
            throw std::runtime_error("Collision rows have inconsistent width: " + path.string());
        }
    }

    if (static_cast<int>(level.fluids.size()) != rows) {
        level.fluids = empty_fluids_grid(cols, rows);
    }
    for (const auto& row : level.fluids) {
        if (static_cast<int>(row.size()) != cols) {
            throw std::runtime_error("Fluids rows have inconsistent width: " + path.string());
        }
    }
}

[[nodiscard]] LevelData load_level_document(const nlohmann::json& json, const std::filesystem::path& path) {
    LevelData level;
    level.name = json.value("name", path.stem().string());
    level.grid_size = json.value("grid_size", 16);
    level.width = json.value("width", 0);
    level.height = json.value("height", 0);
    level.resource_pack = json.value("resource_pack", resources::kDefaultPackId);
    level.background = json.value("background", "");
    level.music = json.value("music", "");

    const int version = json.at("version").get<int>();
    if (version >= 4 && json.contains("tile_layers")) {
        for (const auto& layer_json : json.at("tile_layers")) {
            level.tile_layers.push_back(parse_tile_layer(layer_json));
        }
    } else if (json.contains("tiles")) {
        TileLayer migrated;
        migrated.id = "main";
        migrated.z = 0;
        for (const auto& tile_json : json.at("tiles")) {
            migrated.tiles.push_back(parse_placed_tile(tile_json));
        }
        level.tile_layers.push_back(std::move(migrated));
    }

    if (level.tile_layers.empty()) {
        level.tile_layers.push_back(TileLayer{.id = "main", .z = 0});
    }

    const int cols = sub_grid_cols(level.width, level.grid_size);
    const int rows = sub_grid_rows(level.height, level.grid_size);

    if (json.contains("collision")) {
        level.collision = json.at("collision").get<std::vector<std::vector<int>>>();
    } else {
        level.collision = empty_collision_grid(cols, rows);
    }

    if (json.contains("fluids")) {
        level.fluids = json.at("fluids").get<std::vector<std::vector<int>>>();
    } else {
        level.fluids = empty_fluids_grid(cols, rows);
    }

    validate_collision_and_fluids(level, path);

    if (json.contains("objects")) {
        for (const auto& object_json : json.at("objects")) {
            level.objects.push_back(parse_object(object_json));
        }
    }

    return level;
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

    const int version = json.at("version").get<int>();
    if (version < 3 || version > LevelData::kVersion) {
        throw std::runtime_error(
            "Unsupported level version " + std::to_string(version) + " in: " + path.string() +
            " (expected rivet-level v3–v" + std::to_string(LevelData::kVersion) + ")");
    }

    return load_level_document(json, path);
}

} // namespace rivet::game::level
