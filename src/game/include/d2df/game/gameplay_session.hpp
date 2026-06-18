#pragma once

#include <d2df/ecs/game_world.hpp>
#include <d2df/engine/map/tile_map.hpp>
#include <d2df/engine/render/tile_map_render_index.hpp>
#include <d2df/game/game_map_bundle.hpp>
#include <d2df/sim/map_collision.hpp>
#include <d2df/sim/player_state.hpp>
#include <d2df/sim/trigger_system.hpp>

#include <filesystem>

namespace d2df {

class EventBus;

} // namespace d2df

namespace d2df::game {

/// Full single-player gameplay state: legacy map data + engine tiles + sim systems.
class GameplaySession {
public:
    void load(const std::filesystem::path& map_path);
    void load_bundle(GameMapBundle bundle);

    void fixed_update(sim::PlayerInput input, EventBus* events, bool use_pressed);

    void sync_runtime_tiles();
    void rebuild_tile_render_index();

    [[nodiscard]] GameMapBundle& bundle() { return bundle_; }
    [[nodiscard]] const GameMapBundle& bundle() const { return bundle_; }
    [[nodiscard]] map::MapDocument& legacy_map() { return bundle_.legacy; }
    [[nodiscard]] const map::MapDocument& legacy_map() const { return bundle_.legacy; }
    [[nodiscard]] engine::map::TileMap& runtime_tiles() { return bundle_.runtime; }
    [[nodiscard]] sim::TriggerSystem& triggers() { return triggers_; }
    [[nodiscard]] const sim::TriggerSystem& triggers() const { return triggers_; }
    [[nodiscard]] ecs::GameWorld& world() { return world_; }
    [[nodiscard]] const ecs::GameWorld& world() const { return world_; }
    [[nodiscard]] sim::MapCollision& collision() { return collision_; }
    [[nodiscard]] const sim::MapCollision& collision() const { return collision_; }
    [[nodiscard]] engine::render::TileMapRenderIndex& tile_render_index() {
        return tile_render_index_;
    }

private:
    void rebuild_collision_from_panels();

    GameMapBundle bundle_;
    sim::TriggerSystem triggers_;
    ecs::GameWorld world_;
    sim::MapCollision collision_;
    engine::render::TileMapRenderIndex tile_render_index_;
};

} // namespace d2df::game
