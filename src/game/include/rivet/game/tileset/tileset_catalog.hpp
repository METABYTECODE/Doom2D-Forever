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

enum class TileAnchor {
    TopLeft,
    BottomLeft,
    Center,
};

struct TilesetDef {
    std::string id;
    int tile_width = 8;
    int tile_height = 8;
    int columns = 1;
    TileAnchor anchor = TileAnchor::TopLeft;
    int offset_x = 0;
    int offset_y = 0;
    bool has_explicit_offset = false;
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

/// Source UV rect for a tile index in a uniform atlas grid.
[[nodiscard]] rivet::render::Rect tile_source_rect(const TilesetDef& tileset, int tile_id);

/// World-pixel destination for a placed tile anchor.
[[nodiscard]] rivet::render::Rect tile_dest_rect(
    const TilesetDef& tileset,
    float anchor_x,
    float anchor_y);

} // namespace rivet::game::tileset
