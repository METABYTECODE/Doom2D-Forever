#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include <d2df/core/fixed_timestep.hpp>
#include <d2df/map/area_types.hpp>
#include <d2df/map/map_document.hpp>
#include <d2df/map/map_spawn.hpp>
#include <d2df/map/panel_types.hpp>
#include <d2df/map/trigger_types.hpp>
#include <d2df/sim/map_collision.hpp>
#include <d2df/sim/player_state.hpp>
#include <d2df/sim/trigger_system.hpp>

using namespace d2df;

TEST_CASE("FixedTimestep accumulates 36 UPS steps", "[sim]") {
    FixedTimestep ts;
    ts.advance(1.0f / 36.0f);
    CHECK(ts.consume_fixed_step());
    CHECK_FALSE(ts.consume_fixed_step());
}

TEST_CASE("MapCollision blocks AABB against wall panel", "[sim]") {
    map::MapDocument doc;
    doc.panels.push_back(map::MapPanel{});
    auto& panel = doc.panels.back();
    panel.type = map::PANEL_WALL;
    panel.position = {100, 100};
    panel.size = {32, 32};

    sim::MapCollision collision;
    collision.build_from_map(doc);
    REQUIRE(collision.panels().size() == 1);

    float x = 80.0f;
    float y = 100.0f;
    const float width = 16.0f;
    const float height = 24.0f;
    const auto state = collision.move_object(x, y, width, height, 40, 0, false);
    CHECK((state & sim::MOVE_HITWALL) != 0);
    CHECK(x < 100.0f);
    CHECK(x + width <= 100.0f + 0.01f);
}

TEST_CASE("find_player_spawn prefers AREA_PLAYERPOINT1", "[sim]") {
    map::MapDocument doc;
    doc.areas.push_back({{79, 588}, map::AreaType::PlayerPoint1});
    doc.areas.push_back({{135, 588}, map::AreaType::PlayerPoint2});

    const auto spawn = map::find_player_spawn(doc, 0);
    REQUIRE(spawn.has_value());
    CHECK(spawn->x == 79);
    CHECK(spawn->y == 588);
}

TEST_CASE("PlayerState decays horizontal velocity without input", "[sim]") {
    map::MapDocument doc;
    sim::MapCollision collision;
    collision.build_from_map(doc);

    sim::PlayerState player;
    player.snap_to(100.0f, 100.0f);

    sim::PlayerInput run_right{false, true, false};
    for (int i = 0; i < 12; ++i) {
        player.fixed_update(collision, run_right);
    }

    const float x_after_run = player.x;
    sim::PlayerInput none{};
    for (int i = 0; i < 36; ++i) {
        player.fixed_update(collision, none);
    }
    const float x_after_coast = player.x;

    for (int i = 0; i < 36; ++i) {
        player.fixed_update(collision, none);
    }

    CHECK(x_after_run < x_after_coast);
    CHECK(std::abs(player.x - x_after_coast) < 1.0f);
}

TEST_CASE("door groups exclude PANEL_WALL", "[sim]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& floor = doc.panels.back();
    floor.type = map::PANEL_WALL;
    floor.position = {0, 100};
    floor.size = {256, 16};

    doc.panels.push_back(map::MapPanel{});
    auto& door = doc.panels.back();
    door.type = map::PANEL_CLOSEDOOR;
    door.position = {100, 84};
    door.size = {16, 16};

    doc.triggers.push_back(map::MapTrigger{});
    auto& trigger = doc.triggers.back();
    trigger.type = map::TriggerType::Door;
    trigger.position = {0, 0};
    trigger.size = {32, 32};
    trigger.target_panel = 1;
    trigger.activate = map::ActivateType::PlayerPress;

    sim::TriggerSystem triggers;
    triggers.reset(doc);

    sim::MapCollision collision;
    collision.build_from_panels(triggers.panels());

    sim::PlayerState player;
    player.snap_to(4.0f, 4.0f);
    triggers.update(player, true);
    collision.build_from_panels(triggers.panels());

    CHECK(triggers.panels()[0].enabled);
    CHECK_FALSE(triggers.panels()[1].enabled);

    float x = 110.0f;
    float y = 90.0f;
    const auto state = collision.move_object(x, y, 16.0f, 24.0f, 0, 20, false);
    CHECK((state & sim::MOVE_HITLAND) != 0);
}

TEST_CASE("TRIGGER_DOOR opens grouped door panels", "[sim]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& visible = doc.panels.back();
    visible.type = map::PANEL_CLOSEDOOR;
    visible.position = {100, 100};
    visible.size = {16, 48};
    visible.enabled = true;

    doc.panels.push_back(map::MapPanel{});
    auto& hidden = doc.panels.back();
    hidden.type = map::PANEL_CLOSEDOOR;
    hidden.position = {100, 84};
    hidden.size = {16, 16};
    hidden.flags = map::PANEL_FLAG_HIDE;
    hidden.enabled = true;

    doc.triggers.push_back(map::MapTrigger{});
    auto& trigger = doc.triggers.back();
    trigger.type = map::TriggerType::Door;
    trigger.position = {80, 120};
    trigger.size = {16, 16};
    trigger.target_panel = 1;
    trigger.activate = map::ActivateType::PlayerPress;

    sim::TriggerSystem triggers;
    triggers.reset(doc);

    sim::MapCollision collision;
    collision.build_from_panels(triggers.panels());

    sim::PlayerState player;
    player.snap_to(78.0f, 118.0f);

    triggers.update(player, true);
    collision.build_from_panels(triggers.panels());

    CHECK_FALSE(triggers.panels()[0].enabled);
    CHECK_FALSE(triggers.panels()[1].enabled);

    float x = 90.0f;
    float y = 110.0f;
    const auto state = collision.move_object(x, y, 16.0f, 24.0f, 24, 0, false);
    CHECK((state & sim::MOVE_HITWALL) == 0);
}

TEST_CASE("PANEL_BLOCKMON does not block player movement", "[sim]") {
    map::MapDocument doc;
    doc.panels.push_back(map::MapPanel{});
    auto& blocker = doc.panels.back();
    blocker.type = map::PANEL_BLOCKMON;
    blocker.position = {100, 100};
    blocker.size = {32, 128};

    sim::MapCollision collision;
    collision.build_from_map(doc);

    float x = 80.0f;
    float y = 100.0f;
    const float width = 16.0f;
    const float height = 24.0f;
    const auto state = collision.move_object(x, y, width, height, 40, 0, false);
    CHECK((state & sim::MOVE_HITWALL) == 0);
    CHECK(x > 80.0f);
}

TEST_CASE("vertical lift zone pulls player upward", "[sim]") {
    map::MapDocument doc;
    doc.panels.push_back(map::MapPanel{});
    auto& lift = doc.panels.back();
    lift.type = map::PANEL_LIFTUP;
    lift.position = {0, 0};
    lift.size = {128, 128};

    sim::MapCollision collision;
    collision.build_from_map(doc);

    sim::PlayerState player;
    player.snap_to(32.0f, 64.0f);

    sim::PlayerInput none{};
    const float start_y = player.y;
    for (int i = 0; i < 36; ++i) {
        player.fixed_update(collision, none);
    }

    CHECK(player.y < start_y);
    CHECK((collision.vertical_lift_at(player.x, player.y, player.width, player.height) == -1 ||
           player.y < start_y));
}

TEST_CASE("teleport d2d target is player feet position", "[sim]") {
    map::MapDocument doc;
    doc.triggers.push_back(map::MapTrigger{});
    auto& trigger = doc.triggers.back();
    trigger.type = map::TriggerType::Teleport;
    trigger.teleport_target = {864, 992};
    trigger.d2d = true;
    trigger.activate = map::ActivateType::PlayerPress;
    trigger.position = {0, 0};
    trigger.size = {32, 32};

    sim::TriggerSystem triggers;
    triggers.reset(doc);

    sim::PlayerState player;
    player.snap_to(4.0f, 4.0f);
    triggers.update(player, true);

    CHECK(std::abs(player.x - (864.0f - sim::PlayerState::width * 0.5f)) < 0.01f);
    CHECK(std::abs(player.y - (992.0f - sim::PlayerState::height)) < 0.01f);
}

TEST_CASE("CloseTrap on load leaves trap panels open", "[sim]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& center = doc.panels.back();
    center.type = map::PANEL_OPENDOOR;
    center.position = {100, 100};
    center.size = {16, 48};

    doc.panels.push_back(map::MapPanel{});
    auto& left = doc.panels.back();
    left.type = map::PANEL_OPENDOOR;
    left.position = {200, 100};
    left.size = {16, 48};

    doc.panels.push_back(map::MapPanel{});
    auto& right = doc.panels.back();
    right.type = map::PANEL_OPENDOOR;
    right.position = {300, 100};
    right.size = {16, 48};

    doc.triggers.push_back(map::MapTrigger{});
    auto& open_center = doc.triggers.back();
    open_center.type = map::TriggerType::OpenDoor;
    open_center.target_panel = 0;
    open_center.activate = map::ActivateType::None;

    doc.triggers.push_back(map::MapTrigger{});
    auto& open_sides = doc.triggers.back();
    open_sides.type = map::TriggerType::OpenDoor;
    open_sides.target_panel = 1;
    open_sides.activate = map::ActivateType::None;

    doc.triggers.push_back(map::MapTrigger{});
    auto& close_center = doc.triggers.back();
    close_center.type = map::TriggerType::CloseTrap;
    close_center.target_panel = 0;
    close_center.activate = map::ActivateType::None;

    sim::TriggerSystem triggers;
    triggers.reset(doc);

    CHECK(triggers.panels()[0].enabled);
    CHECK_FALSE(triggers.panels()[1].enabled);
    CHECK_FALSE(triggers.panels()[2].enabled);
}

TEST_CASE("CloseDoor trigger makes OPENDOOR panel solid", "[sim]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& platform = doc.panels.back();
    platform.type = map::PANEL_OPENDOOR;
    platform.position = {100, 80};
    platform.size = {16, 16};

    doc.triggers.push_back(map::MapTrigger{});
    auto& trigger = doc.triggers.back();
    trigger.type = map::TriggerType::CloseDoor;
    trigger.target_panel = 0;
    trigger.position = {80, 80};
    trigger.size = {16, 16};
    trigger.activate = map::ActivateType::PlayerPress;

    sim::TriggerSystem triggers;
    triggers.reset(doc);

    sim::MapCollision collision;
    collision.build_from_panels(triggers.panels());

    sim::PlayerState player;
    player.snap_to(78.0f, 78.0f);
    triggers.update(player, true);
    collision.build_from_panels(triggers.panels());

    CHECK(triggers.panels()[0].enabled);

    float x = 90.0f;
    float y = 70.0f;
    const auto state = collision.move_object(x, y, 16.0f, 24.0f, 0, 20, false);
    CHECK((state & sim::MOVE_HITLAND) != 0);
}

TEST_CASE("open OPENDOOR panel has no collision", "[sim]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& spike = doc.panels.back();
    spike.type = map::PANEL_OPENDOOR;
    spike.position = {100, 80};
    spike.size = {16, 48};
    spike.enabled = false;

    sim::MapCollision collision;
    collision.build_from_panels(doc.panels);

    float x = 90.0f;
    float y = 100.0f;
    const auto state = collision.move_object(x, y, 16.0f, 24.0f, 24, 0, false);
    CHECK((state & sim::MOVE_HITWALL) == 0);
}
