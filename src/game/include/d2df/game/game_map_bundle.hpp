#pragma once

#include <filesystem>

#include <d2df/engine/map/tile_map.hpp>
#include <d2df/map/map_document.hpp>

namespace d2df::game {

/// Loaded map for the game client: legacy data for gameplay + engine tile runtime.
struct GameMapBundle {
    map::MapDocument legacy;
    engine::map::TileMap runtime;
};

/// Load a legacy map JSON once; produce both gameplay document and engine tile map.
[[nodiscard]] GameMapBundle load_legacy_map(const std::filesystem::path& path);

} // namespace d2df::game
