#pragma once

#include <d2df/map/map_document.hpp>

#include <optional>

namespace d2df::map {

/// Returns spawn position for player hitbox top-left (legacy: area pos = PLAYER_RECT origin).
[[nodiscard]] std::optional<MapPoint> find_player_spawn(const MapDocument& map, int player_slot = 0);

} // namespace d2df::map
