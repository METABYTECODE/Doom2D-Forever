#include <d2df/game/gameplay_session.hpp>

#include <d2df/core/event_bus.hpp>
#include <d2df/engine/map/tile_map_sync.hpp>

namespace d2df::game {

void GameplaySession::load(const std::filesystem::path& map_path) {
    load_bundle(load_legacy_map(map_path));
}

void GameplaySession::load_bundle(GameMapBundle bundle) {
    bundle_ = std::move(bundle);
    triggers_.reset(bundle_.legacy);
    rebuild_collision_from_panels();
    world_.reset_player(bundle_.legacy);
    sync_runtime_tiles();
    rebuild_tile_render_index();
}

void GameplaySession::rebuild_collision_from_panels() {
    collision_.build_from_panels(triggers_.panels());
}

void GameplaySession::sync_runtime_tiles() {
    engine::map::sync_tiles_from_panels(bundle_.runtime, triggers_.panels());
}

void GameplaySession::rebuild_tile_render_index() {
    tile_render_index_.build(bundle_.runtime);
}

void GameplaySession::fixed_update(sim::PlayerInput input, EventBus* events, bool use_pressed) {
    rebuild_collision_from_panels();
    world_.fixed_update(collision_, input, events, bundle_.legacy.size.width, bundle_.legacy.size.height,
                        &triggers_);
    triggers_.update(world_.player(), use_pressed, events, &world_.targets(), &collision_);
    rebuild_collision_from_panels();
    sync_runtime_tiles();
}

} // namespace d2df::game
