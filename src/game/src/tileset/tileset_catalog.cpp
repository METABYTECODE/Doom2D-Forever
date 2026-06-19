#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include <rivet/game/tileset/tileset_catalog.hpp>
#include <rivet/resources/resource_manager.hpp>

namespace rivet::game::tileset {

namespace {

[[nodiscard]] TilesetDef parse_tileset_json(
    const std::filesystem::path& json_path,
    rivet::resources::ResourceManager& resources) {
    std::ifstream input(json_path);
    if (!input) {
        throw std::runtime_error("Failed to open tileset: " + json_path.string());
    }

    const auto json = nlohmann::json::parse(input);
    TilesetDef def;
    def.id = json.value("id", json_path.stem().string());
    def.tile_width = json.value("tile_width", 8);
    def.tile_height = json.value("tile_height", 8);
    def.columns = json.value("columns", 1);

    if (json.contains("offset_x") || json.contains("offset_y")) {
        def.offset_x = json.value("offset_x", 0);
        def.offset_y = json.value("offset_y", 0);
        def.has_explicit_offset = true;
    } else if (json.contains("anchor")) {
        const std::string anchor = json.at("anchor").get<std::string>();
        if (anchor == "bottom-left") {
            def.anchor = TileAnchor::BottomLeft;
        } else if (anchor == "center") {
            def.anchor = TileAnchor::Center;
        }
    }

    const std::string texture_name = json.value("texture", def.id + ".png");
    const auto texture_path = json_path.parent_path() / texture_name;
    def.texture = resources.load_texture(texture_path);
    if (def.texture == rivet::resources::kInvalidTexture) {
        throw std::runtime_error("Failed to load tileset texture: " + texture_path.string());
    }

    return def;
}

} // namespace

TilesetCatalog::TilesetCatalog(
    rivet::resources::ResourceManager& resources,
    const std::filesystem::path& tilesets_dir) : resources_(resources) {
    if (!std::filesystem::exists(tilesets_dir)) {
        return;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(tilesets_dir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") {
            continue;
        }
        try {
            TilesetDef def = parse_tileset_json(entry.path(), resources_);
            tilesets_[def.id] = def;
        } catch (const std::exception&) {
            // Skip broken tilesets; game can still run with partial content.
        }
    }
}

const TilesetDef* TilesetCatalog::find(const std::string& id) const {
    const auto it = tilesets_.find(id);
    if (it == tilesets_.end()) {
        return nullptr;
    }
    return &it->second;
}

rivet::render::Rect tile_source_rect(const TilesetDef& tileset, const int tile_id) {
    const int col = tile_id % tileset.columns;
    const int row = tile_id / tileset.columns;
    return {
        .x = static_cast<float>(col * tileset.tile_width),
        .y = static_cast<float>(row * tileset.tile_height),
        .width = static_cast<float>(tileset.tile_width),
        .height = static_cast<float>(tileset.tile_height),
    };
}

rivet::render::Rect tile_dest_rect(
    const TilesetDef& tileset,
    const float anchor_x,
    const float anchor_y) {
    float draw_x = anchor_x;
    float draw_y = anchor_y;

    if (tileset.has_explicit_offset) {
        draw_x += static_cast<float>(tileset.offset_x);
        draw_y += static_cast<float>(tileset.offset_y);
    } else {
        switch (tileset.anchor) {
        case TileAnchor::BottomLeft:
            draw_y -= static_cast<float>(tileset.tile_height);
            break;
        case TileAnchor::Center:
            draw_x -= static_cast<float>(tileset.tile_width) * 0.5f;
            draw_y -= static_cast<float>(tileset.tile_height) * 0.5f;
            break;
        case TileAnchor::TopLeft:
        default:
            break;
        }
    }

    return {
        .x = draw_x,
        .y = draw_y,
        .width = static_cast<float>(tileset.tile_width),
        .height = static_cast<float>(tileset.tile_height),
    };
}

} // namespace rivet::game::tileset
