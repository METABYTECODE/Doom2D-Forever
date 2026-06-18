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

[[nodiscard]] std::vector<std::vector<int>> empty_collision_grid(const int width, const int height) {
    return std::vector<std::vector<int>>(
        static_cast<std::size_t>(height),
        std::vector<int>(static_cast<std::size_t>(width), 0));
}

[[nodiscard]] LevelData load_level_v1(const nlohmann::json& json, const std::filesystem::path& path) {
    LevelData level;
    level.name = json.value("name", path.stem().string());

    const int legacy_tile_size = json.value("tile_size", 32);
    level.grid_size = 8;
    const int scale = legacy_tile_size / level.grid_size;
    if (scale < 1 || legacy_tile_size % level.grid_size != 0) {
        throw std::runtime_error("Legacy tile_size must be a multiple of 8: " + path.string());
    }

    const auto legacy_tiles = json.at("tiles").get<std::vector<std::vector<int>>>();
    const int legacy_height = json.value("height", static_cast<int>(legacy_tiles.size()));
    const int legacy_width =
        json.value("width", legacy_tiles.empty() ? 0 : static_cast<int>(legacy_tiles.front().size()));

    level.width = legacy_width * scale;
    level.height = legacy_height * scale;
    level.collision = empty_collision_grid(level.width, level.height);

    for (int y = 0; y < legacy_height; ++y) {
        for (int x = 0; x < legacy_width; ++x) {
            if (legacy_tiles[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] != 1) {
                continue;
            }
            for (int dy = 0; dy < scale; ++dy) {
                for (int dx = 0; dx < scale; ++dx) {
                    level.collision[static_cast<std::size_t>(y * scale + dy)]
                                   [static_cast<std::size_t>(x * scale + dx)] = 1;
                }
            }
        }
    }

    if (json.contains("objects")) {
        for (const auto& object_json : json.at("objects")) {
            level.objects.push_back(parse_object(object_json));
        }
    }

    return level;
}

[[nodiscard]] LevelData load_level_v2(const nlohmann::json& json, const std::filesystem::path& path) {
    LevelData level;
    level.name = json.value("name", path.stem().string());
    level.grid_size = json.value("grid_size", 8);
    level.width = json.value("width", 0);
    level.height = json.value("height", 0);
    level.background = json.value("background", "");
    level.music = json.value("music", "");

    if (json.contains("tiles")) {
        for (const auto& tile_json : json.at("tiles")) {
            level.tiles.push_back(parse_placed_tile(tile_json));
        }
    }

    if (json.contains("collision")) {
        level.collision = json.at("collision").get<std::vector<std::vector<int>>>();
    } else {
        level.collision = empty_collision_grid(level.width, level.height);
    }

    if (level.height == 0) {
        level.height = static_cast<int>(level.collision.size());
    }
    if (level.width == 0 && !level.collision.empty()) {
        level.width = static_cast<int>(level.collision.front().size());
    }

    for (const auto& row : level.collision) {
        if (static_cast<int>(row.size()) != level.width) {
            throw std::runtime_error("Collision rows have inconsistent width: " + path.string());
        }
    }

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
    if (version == 1) {
        return load_level_v1(json, path);
    }
    if (version == LevelData::kVersion) {
        return load_level_v2(json, path);
    }

    throw std::runtime_error("Unsupported level version in: " + path.string());
}

} // namespace rivet::game::level
