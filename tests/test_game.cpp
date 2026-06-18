#include <catch2/catch_test_macros.hpp>

#include <d2df/engine/map/legacy_map_adapter.hpp>
#include <d2df/engine/map/tile_flags.hpp>
#include <d2df/engine/map/tile_map_sync.hpp>
#include <d2df/game/game_map_bundle.hpp>
#include <d2df/game/gameplay_session.hpp>
#include <d2df/map/panel_types.hpp>

TEST_CASE("load_legacy_map produces runtime tile map from legacy document", "[game]") {
    d2df::map::MapDocument legacy;
    legacy.id = "test";
    legacy.size.width = 400;
    legacy.size.height = 300;

    d2df::map::MapPanel floor;
    floor.type = d2df::map::PANEL_WALL;
    floor.position = {0, 250};
    floor.size = {400, 50};
    legacy.panels.push_back(floor);

    d2df::game::GameMapBundle bundle;
    bundle.legacy = std::move(legacy);
    bundle.runtime = d2df::engine::map::tile_map_from_legacy(bundle.legacy);

    CHECK(bundle.runtime.id == "test");
    CHECK(bundle.runtime.width == 400);
    CHECK(bundle.runtime.layers.size() == 1);
    CHECK(bundle.runtime.layers[0].tiles.size() == 1);
    CHECK(bundle.runtime.layers[0].tiles[0].flags == d2df::engine::map::TILE_WALL);
}

TEST_CASE("sync_tiles_from_panels updates door enabled state", "[game]") {
    d2df::engine::map::TileMap tile_map;
    d2df::engine::map::TileLayer layer;
    layer.name = "legacy_panels";

    d2df::engine::map::TilePlacement tile;
    tile.x = 10;
    tile.y = 20;
    tile.width = 64;
    tile.height = 32;
    tile.flags = d2df::engine::map::TILE_CLOSE_DOOR;
    tile.enabled = true;
    layer.tiles.push_back(tile);
    tile_map.layers.push_back(layer);

    std::vector<d2df::map::MapPanel> panels(1);
    panels[0].position = {10, 20};
    panels[0].size = {64, 32};
    panels[0].type = d2df::map::PANEL_CLOSEDOOR;
    panels[0].enabled = false;

    d2df::engine::map::sync_tiles_from_panels(tile_map, panels);

    CHECK_FALSE(tile_map.layers[0].tiles[0].enabled);
}

TEST_CASE("GameplaySession loads bundle and moves player", "[game]") {
    d2df::map::MapDocument legacy;
    legacy.size.width = 640;
    legacy.size.height = 480;

    d2df::map::MapPanel floor;
    floor.type = d2df::map::PANEL_WALL;
    floor.position = {0, 400};
    floor.size = {640, 80};
    legacy.panels.push_back(floor);

    d2df::map::MapArea spawn;
    spawn.type = d2df::map::AreaType::PlayerPoint1;
    spawn.position = {80, 300};
    legacy.areas.push_back(spawn);

    d2df::game::GameMapBundle bundle;
    bundle.legacy = std::move(legacy);
    bundle.runtime = d2df::engine::map::tile_map_from_legacy(bundle.legacy);

    d2df::game::GameplaySession session;
    session.load_bundle(std::move(bundle));

    const auto start_x = session.world().player().x;

    d2df::sim::PlayerInput move;
    move.right = true;
    for (int i = 0; i < 24; ++i) {
        session.fixed_update(move, nullptr, false);
    }

    CHECK(session.world().player().x > start_x);
}
