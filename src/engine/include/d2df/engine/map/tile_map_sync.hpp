#pragma once

#include <vector>

#include <d2df/engine/map/tile_map.hpp>
#include <d2df/map/map_document.hpp>

namespace d2df::engine::map {

/// Copies live panel state into the engine tile map (doors, lifts, enabled flags).
void sync_tiles_from_panels(TileMap& tile_map, const std::vector<::d2df::map::MapPanel>& panels);

} // namespace d2df::engine::map
