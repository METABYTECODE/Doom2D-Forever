#include <catch2/catch_test_macros.hpp>

#include <d2df/map/map_document.hpp>
#include <d2df/map/panel_types.hpp>
#include <d2df/engine/map/legacy_map_adapter.hpp>
#include <d2df/engine/map/map_loader.hpp>
#include <d2df/engine/map/tile_flags.hpp>
#include <d2df/engine/map/tile_map_json.hpp>
#include <d2df/engine/runtime/map_session.hpp>

namespace {

d2df::engine::map::TileMap make_floor_map() {
    d2df::engine::map::TileMap map;
    map.width = 800;
    map.height = 600;

    d2df::engine::map::TileLayer layer;
    layer.name = "floor";
    d2df::engine::map::TilePlacement floor;
    floor.x = 0;
    floor.y = 500;
    floor.width = 800;
    floor.height = 100;
    floor.flags = d2df::engine::map::TILE_WALL;
    layer.tiles.push_back(floor);
    map.layers.push_back(layer);

    map.spawns.push_back({"player1", 100, 400});
    return map;
}

} // namespace

TEST_CASE("MapSession loads tile map and moves player on solid floor", "[engine]") {
    d2df::engine::MapSession session;
    session.load(make_floor_map());

    const auto start_y = session.world().player().y;

    d2df::engine::MovementInput move_right;
    move_right.right = true;

    for (int i = 0; i < 36; ++i) {
        session.fixed_tick(move_right);
    }

    CHECK(session.world().player().x > 100.0f);
    CHECK(session.world().player().on_ground());

    d2df::engine::MovementInput jump;
    jump.jump = true;
    for (int i = 0; i < 8; ++i) {
        session.fixed_tick(jump);
    }

    CHECK(session.world().player().y < start_y);
}

TEST_CASE("MapSession spawns entity bodies from tile map", "[engine]") {
    auto map = make_floor_map();
    map.entities.push_back({"zomby", 120, 80, 0.0f, 0.0f});

    d2df::engine::MapSession session;
    session.load(std::move(map));

    CHECK(session.world().entity_entities().size() == 1);

    const auto entity = session.world().entity_entities().front();
    const auto& registry = session.world().registry();
    CHECK(registry.all_of<d2df::engine::ecs::Collider>(entity));
    CHECK(registry.all_of<d2df::engine::ecs::MonsterTag>(entity));
}

TEST_CASE("Player collides with solid map entity", "[engine]") {
    auto map = make_floor_map();
    map.entities.push_back({"zomby", 150, 400, 34.0f, 52.0f});

    d2df::engine::MapSession session;
    session.load(std::move(map));

    d2df::engine::MovementInput move_right;
    move_right.right = true;

    for (int i = 0; i < 12; ++i) {
        session.fixed_tick(move_right);
    }

    const auto& player = session.world().player();
    CHECK(player.x + player.width <= 150.0f + 1.0f);
}

TEST_CASE("Native tile map JSON loads format d2df-tilemap", "[engine]") {
    const std::string json = R"({
        "format": "d2df-tilemap",
        "version": 1,
        "id": "test",
        "world": { "width": 320, "height": 240 },
        "layers": [
            {
                "name": "walls",
                "tiles": [
                    { "x": 0, "y": 200, "w": 320, "h": 40, "flags": ["wall"] }
                ]
            }
        ],
        "spawns": [ { "id": "player1", "x": 40, "y": 120 } ],
        "entities": [ { "type": "imp", "x": 80, "y": 100 } ]
    })";

    const auto map = d2df::engine::map::load_tile_map_json_string(json);
    CHECK(map.id == "test");
    CHECK(map.width == 320);
    CHECK(map.layers.size() == 1);
    CHECK(map.layers[0].tiles.size() == 1);
    CHECK(map.spawns.size() == 1);
    CHECK(map.entities.size() == 1);
    CHECK(map.entities[0].height > 0.0f);
}

TEST_CASE("Legacy map document converts to tile map", "[engine]") {
    d2df::map::MapDocument legacy;
    legacy.size.width = 200;
    legacy.size.height = 150;
    legacy.panels.push_back({});
    legacy.panels.back().position = {0, 100};
    legacy.panels.back().size = {200, 50};
    legacy.panels.back().type = d2df::map::PANEL_WALL;

    const auto tile_map = d2df::engine::map::tile_map_from_legacy(legacy);
    CHECK(tile_map.width == 200);
    CHECK(tile_map.layers.size() == 1);
    CHECK(tile_map.layers[0].tiles.size() == 1);
}
