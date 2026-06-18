#include <d2df/engine/runtime/map_session.hpp>

namespace d2df::engine {

void MapSession::load(map::TileMap tile_map) {
    tile_map_ = std::move(tile_map);
    collision_.build(tile_map_);
    world_.reset(tile_map_);
}

void MapSession::fixed_tick(MovementInput input) {
    world_.fixed_tick(collision_, input);
}

} // namespace d2df::engine
