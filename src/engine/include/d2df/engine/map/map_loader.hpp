#pragma once

#include <filesystem>

#include <d2df/engine/map/tile_map.hpp>

namespace d2df::engine::map {

/// Loads native tile maps or converts legacy map JSON on read.
[[nodiscard]] TileMap load_map_file(const std::filesystem::path& path);

} // namespace d2df::engine::map
