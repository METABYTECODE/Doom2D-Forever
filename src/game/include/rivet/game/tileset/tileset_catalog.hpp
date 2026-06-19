#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

#include <rivet/render/rect.hpp>
#include <rivet/resources/texture.hpp>

namespace rivet::resources {
class ResourceManager;
}

namespace rivet::game::tileset {

struct TilesetDef {
    std::string id;
    int tile_width = 8;
    int tile_height = 8;
    int columns = 1;
    rivet::resources::TextureHandle texture = rivet::resources::kInvalidTexture;
};

/// Loads tileset JSON + textures from `content/textures/tilesets/` (recursive).
class TilesetCatalog {
public:
    TilesetCatalog(
        rivet::resources::ResourceManager& resources,
        const std::filesystem::path& tilesets_dir);

    [[nodiscard]] const TilesetDef* find(const std::string& id) const;

private:
    rivet::resources::ResourceManager& resources_;
    std::unordered_map<std::string, TilesetDef> tilesets_;
};

/// Grid cells occupied by a tile graphic (matches editor `tileCellSpan`).
[[nodiscard]] int tile_cell_span(int tile_pixels, int grid_size);

[[nodiscard]] rivet::render::Rect tile_source_rect(const TilesetDef& tileset, int tile_id);

} // namespace rivet::game::tileset
