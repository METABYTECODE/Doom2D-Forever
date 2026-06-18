#pragma once

#include <filesystem>
#include <optional>

#include <d2df/map/map_document.hpp>
#include <d2df/engine/map/tile_map.hpp>

namespace d2df::engine::map {

[[nodiscard]] TileMap tile_map_from_legacy(const ::d2df::map::MapDocument& legacy);

[[nodiscard]] std::optional<SpawnPoint> find_spawn(const TileMap& map,
                                                   std::string_view spawn_id = "player1");

} // namespace d2df::engine::map
