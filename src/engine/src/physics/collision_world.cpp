#include <d2df/engine/physics/collision_world.hpp>

#include <d2df/engine/map/tile_flags.hpp>
#include <d2df/map/panel_types.hpp>

namespace d2df::engine::physics {
namespace {

::d2df::map::MapPanel tile_to_panel(const map::TilePlacement& tile) {
    ::d2df::map::MapPanel panel;
    panel.position.x = tile.x;
    panel.position.y = tile.y;
    panel.size.width = tile.width;
    panel.size.height = tile.height;
    panel.texture = tile.tile_index;
    panel.type = static_cast<std::uint16_t>(tile.flags);
    panel.alpha = tile.alpha;
    panel.flags = tile.visibility;
    panel.enabled = tile.enabled;
    return panel;
}

bool include_tile_in_collision(const map::TilePlacement& tile) {
    if (tile.width <= 0 || tile.height <= 0) {
        return false;
    }
    if (!map::tile_participates_in_collision(tile.flags, tile.visibility, tile.enabled)) {
        return false;
    }
    return true;
}

} // namespace

void CollisionWorld::build(const map::TileMap& tile_map) {
    std::vector<::d2df::map::MapPanel> panels;
    for (const auto& layer : tile_map.layers) {
        for (const auto& tile : layer.tiles) {
            if (!include_tile_in_collision(tile)) {
                continue;
            }
            panels.push_back(tile_to_panel(tile));
        }
    }
    collision_.build_from_panels(panels);
}

} // namespace d2df::engine::physics
