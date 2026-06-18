#pragma once

#include <filesystem>

#include <d2df/engine/map/tile_map.hpp>

namespace d2df::engine::map {

[[nodiscard]] TileMap load_tile_map_json(const std::filesystem::path& path);
[[nodiscard]] TileMap load_tile_map_json_string(std::string_view json);

} // namespace d2df::engine::map
