#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>

#include <d2df/core/types.hpp>
#include <d2df/core/event_bus.hpp>
#include <d2df/core/fixed_timestep.hpp>
#include <d2df/core/game_events.hpp>
#include <d2df/ecs/game_world.hpp>
#include <d2df/map/item_types.hpp>
#include <d2df/map/key_types.hpp>
#include <d2df/map/map_document.hpp>
#include <d2df/map/map_json_loader.hpp>
#include <d2df/map/map_spawn.hpp>
#include <d2df/map/monster_types.hpp>
#include <d2df/map/panel_types.hpp>
#include <d2df/map/trigger_types.hpp>

#ifndef D2DF_SOURCE_DIR
#define D2DF_SOURCE_DIR "."
#endif
#include <d2df/sim/item_system.hpp>
#include <d2df/sim/game_rules.hpp>
#include <d2df/sim/monster_system.hpp>
#include <d2df/sim/map_collision.hpp>
#include <d2df/sim/player_state.hpp>
#include <d2df/sim/projectile_system.hpp>
#include <d2df/sim/shootable_target.hpp>
#include <d2df/sim/trigger_system.hpp>
#include <d2df/sim/weapon_system.hpp>
#include <d2df/sim/weapon_types.hpp>

using namespace d2df;

namespace {

void give_ammo(sim::PlayerCombat& combat, sim::AmmoType type, int amount) {
    combat.ammo[static_cast<std::size_t>(type)] = amount;
}

void own_weapon(sim::PlayerCombat& combat, sim::WeaponId weapon) {
    combat.owned[static_cast<std::size_t>(weapon)] = true;
}

} // namespace

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

TEST_CASE("PANEL_BLOCKMON blocks monster movement", "[sim][monsters]") {
    map::MapDocument doc;
    doc.panels.push_back(map::MapPanel{});
    auto& blocker = doc.panels.back();
    blocker.type = map::PANEL_BLOCKMON;
    blocker.position = {100, 100};
    blocker.size = {32, 128};

    sim::MapCollision collision;
    collision.build_from_map(doc);

    const float width = 34.0f;
    const float height = 52.0f;
    float x = 60.0f;
    float y = 100.0f;
    const auto state = collision.move_monster(x, y, width, height, 40, 0, false);
    CHECK((state & sim::MOVE_BLOCK) != 0);
    CHECK(x <= 66.0f);
    CHECK(x + width <= 101.0f);
}

TEST_CASE("zomby death drops ammo clip", "[sim][monsters]") {
    ecs::GameWorld world;
    map::MapDocument doc;

    std::vector<sim::ShootableTarget>& targets = world.targets();
    sim::ShootableTarget zomby;
    zomby.id = 200;
    zomby.monster_type = map::MonsterType::Zomby;
    zomby.x = 200.0f;
    zomby.y = 200.0f;
    zomby.prev_x = zomby.x;
    zomby.prev_y = zomby.y;
    zomby.width = 34.0f;
    zomby.height = 52.0f;
    zomby.max_health = 15;
    zomby.health = 15;
    targets.push_back(zomby);

    sim::MapCollision collision;
    sim::PlayerInput none{};
    targets.front().apply_damage(15, d2df::kPlayerEntityId);
    world.fixed_update(collision, none, nullptr, 512, 512, nullptr);

    CHECK(std::any_of(world.items().items().begin(), world.items().items().end(),
                      [](const sim::WorldItem& item) {
                          return item.active && item.type == map::ItemType::AmmoBullets;
                      }));
}

TEST_CASE("sergeant death drops shotgun", "[sim][monsters]") {
    ecs::GameWorld world;
    map::MapDocument doc;

    std::vector<sim::ShootableTarget>& targets = world.targets();
    sim::ShootableTarget serg;
    serg.id = 201;
    serg.monster_type = map::MonsterType::Serg;
    serg.x = 200.0f;
    serg.y = 200.0f;
    serg.prev_x = serg.x;
    serg.prev_y = serg.y;
    serg.width = 34.0f;
    serg.height = 52.0f;
    serg.max_health = 30;
    serg.health = 30;
    targets.push_back(serg);

    sim::MapCollision collision;
    sim::PlayerInput none{};
    targets.front().apply_damage(30, d2df::kPlayerEntityId);
    world.fixed_update(collision, none, nullptr, 512, 512, nullptr);

    CHECK(std::any_of(world.items().items().begin(), world.items().items().end(),
                      [](const sim::WorldItem& item) {
                          return item.active && item.type == map::ItemType::WeaponShotgun1;
                      }));
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

TEST_CASE("Trap with ACTIVATE_NONE does not fire on map load", "[sim]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& trap = doc.panels.back();
    trap.type = map::PANEL_OPENDOOR;
    trap.position = {944, 272};
    trap.size = {16, 16};

    doc.triggers.push_back(map::MapTrigger{});
    auto& trigger = doc.triggers.back();
    trigger.type = map::TriggerType::Trap;
    trigger.target_panel = 0;
    trigger.position = {944, 272};
    trigger.size = {16, 16};
    trigger.activate = map::ActivateType::None;

    sim::TriggerSystem triggers;
    triggers.reset(doc);

    CHECK_FALSE(triggers.panels()[0].enabled);
}

TEST_CASE("Press expander on load configures trap corridor", "[sim]") {
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
    open_center.position = {100, 100};
    open_center.size = {16, 16};
    open_center.activate = map::ActivateType::None;

    doc.triggers.push_back(map::MapTrigger{});
    auto& open_sides = doc.triggers.back();
    open_sides.type = map::TriggerType::OpenDoor;
    open_sides.target_panel = 1;
    open_sides.position = {200, 100};
    open_sides.size = {16, 16};
    open_sides.activate = map::ActivateType::None;

    doc.triggers.push_back(map::MapTrigger{});
    auto& close_center = doc.triggers.back();
    close_center.type = map::TriggerType::CloseTrap;
    close_center.target_panel = 0;
    close_center.position = {100, 120};
    close_center.size = {16, 16};
    close_center.activate = map::ActivateType::None;

    doc.triggers.push_back(map::MapTrigger{});
    auto& press = doc.triggers.back();
    press.type = map::TriggerType::Press;
    press.position = {80, 80};
    press.size = {16, 16};
    press.activate = map::ActivateType::None;
    press.press_position = {90, 95};
    press.press_size = {230, 30};

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

TEST_CASE("lift direction change preserves door panel state", "[sim]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& door = doc.panels.back();
    door.type = map::PANEL_OPENDOOR;
    door.position = {100, 80};
    door.size = {16, 16};

    doc.panels.push_back(map::MapPanel{});
    auto& lift_zone = doc.panels.back();
    lift_zone.type = map::PANEL_LIFTUP;
    lift_zone.position = {200, 80};
    lift_zone.size = {64, 64};

    doc.triggers.push_back(map::MapTrigger{});
    auto& open_door = doc.triggers.back();
    open_door.type = map::TriggerType::OpenDoor;
    open_door.target_panel = 0;
    open_door.position = {80, 80};
    open_door.size = {16, 16};
    open_door.activate = map::ActivateType::PlayerPress;

    doc.triggers.push_back(map::MapTrigger{});
    auto& lift_toggle = doc.triggers.back();
    lift_toggle.type = map::TriggerType::Lift;
    lift_toggle.target_panel = 1;
    lift_toggle.position = {210, 90};
    lift_toggle.size = {16, 16};
    lift_toggle.activate = map::ActivateType::PlayerPress;

    sim::TriggerSystem triggers;
    triggers.reset(doc);

    sim::PlayerState player;
    player.snap_to(78.0f, 78.0f);
    triggers.update(player, true);
    CHECK_FALSE(triggers.panels()[0].enabled);

    player.snap_to(208.0f, 88.0f);
    triggers.update(player, true);

    CHECK_FALSE(triggers.panels()[0].enabled);
    CHECK((triggers.panels()[1].type & map::PANEL_LIFTDOWN) != 0);
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

TEST_CASE("acid_damage matches legacy g_GetAcidHit table", "[sim]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& acid1 = doc.panels.back();
    acid1.type = map::PANEL_ACID1;
    acid1.position = {100, 100};
    acid1.size = {64, 64};

    sim::MapCollision collision;
    collision.build_from_map(doc);

    CHECK(collision.acid_damage(110.0f, 110.0f, 16.0f, 24.0f) == 5);

    doc.panels.push_back(map::MapPanel{});
    auto& acid2 = doc.panels.back();
    acid2.type = map::PANEL_ACID2;
    acid2.position = {100, 100};
    acid2.size = {64, 64};

    collision.build_from_map(doc);
    CHECK(collision.acid_damage(110.0f, 110.0f, 16.0f, 24.0f) == 20);
}

TEST_CASE("trap closes on player and applies lethal damage", "[sim]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& spike = doc.panels.back();
    spike.type = map::PANEL_OPENDOOR;
    spike.position = {100, 100};
    spike.size = {16, 48};

    doc.triggers.push_back(map::MapTrigger{});
    auto& trap = doc.triggers.back();
    trap.type = map::TriggerType::CloseTrap;
    trap.target_panel = 0;
    trap.position = {90, 120};
    trap.size = {16, 16};
    trap.activate = map::ActivateType::PlayerPress;

    sim::TriggerSystem triggers;
    triggers.reset(doc);

    sim::PlayerState player;
    player.snap_to(102.0f, 110.0f);
    triggers.update(player, true);

    CHECK_FALSE(player.alive());
    CHECK(triggers.panels()[0].enabled);
}

TEST_CASE("exit trigger requests map change", "[sim]") {
    map::MapDocument doc;

    doc.triggers.push_back(map::MapTrigger{});
    auto& exit_trigger = doc.triggers.back();
    exit_trigger.type = map::TriggerType::Exit;
    exit_trigger.position = {0, 0};
    exit_trigger.size = {32, 32};
    exit_trigger.activate = map::ActivateType::PlayerCollide;

    sim::TriggerSystem triggers;
    triggers.reset(doc);

    sim::PlayerState player;
    player.snap_to(4.0f, 4.0f);
    triggers.update(player, false);

    CHECK(triggers.consume_exit_request());
    CHECK_FALSE(triggers.consume_exit_request());
}

TEST_CASE("GameWorld publishes PlayerLanded on ground contact", "[sim]") {
    map::MapDocument doc;
    doc.size = {256, 256};
    doc.panels.push_back(map::MapPanel{});
    auto& floor = doc.panels.back();
    floor.type = map::PANEL_WALL;
    floor.position = {0, 200};
    floor.size = {256, 16};

    sim::MapCollision collision;
    collision.build_from_map(doc);

    ecs::GameWorld world;
    world.reset_player(doc);
    world.player().snap_to(100.0f, 120.0f);

    d2df::EventBus bus;
    int landed_count = 0;
    bus.subscribe<events::PlayerLanded>([&](const events::PlayerLanded&) { ++landed_count; });

    sim::PlayerInput none{};
    for (int i = 0; i < 120; ++i) {
        world.fixed_update(collision, none, &bus, doc.size.width, doc.size.height, nullptr);
    }

    CHECK(landed_count >= 1);
}

TEST_CASE("floor trap closes on ACTIVATE_PLAYERCOLLIDE touch", "[sim]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& spike = doc.panels.back();
    spike.type = map::PANEL_OPENDOOR;
    spike.position = {480, 592};
    spike.size = {16, 48};

    doc.triggers.push_back(map::MapTrigger{});
    auto& trap = doc.triggers.back();
    trap.type = map::TriggerType::Trap;
    trap.target_panel = 0;
    trap.position = {480, 624};
    trap.size = {16, 16};
    trap.activate = map::ActivateType::PlayerCollide;

    sim::TriggerSystem triggers;
    triggers.reset(doc);

    sim::PlayerState player;
    player.snap_to(471.0f, 588.0f);
    triggers.update(player, false);

    CHECK(triggers.panels()[0].enabled);
}

TEST_CASE("veteran map08 trap corridor activates on touch", "[sim][integration]") {
    const auto map_path =
        std::filesystem::path(D2DF_SOURCE_DIR) / "assets/content/maps/veteran/map08.json";
    if (!std::filesystem::exists(map_path)) {
        SKIP("veteran map08 not available");
    }

    const auto doc = map::load_map_json_v1(map_path);
    sim::TriggerSystem triggers;
    triggers.reset(doc);

    sim::PlayerState player;
    player.snap_to(471.0f, 588.0f);
    triggers.update(player, false);

    CHECK(triggers.panels()[301].enabled);
}

TEST_CASE("veteran map08 player walking onto trap activates collide", "[sim][integration]") {
    const auto map_path =
        std::filesystem::path(D2DF_SOURCE_DIR) / "assets/content/maps/veteran/map08.json";
    if (!std::filesystem::exists(map_path)) {
        SKIP("veteran map08 not available");
    }

    const auto doc = map::load_map_json_v1(map_path);
    sim::TriggerSystem triggers;
    triggers.reset(doc);

    sim::MapCollision collision;
    collision.build_from_panels(triggers.panels());

    sim::PlayerState player;
    player.snap_to(471.0f, 588.0f);

    sim::PlayerInput walk_right{false, true, false};
    bool trap_fired = false;

    for (int i = 0; i < 30; ++i) {
        collision.build_from_panels(triggers.panels());
        player.fixed_update(collision, walk_right);
        triggers.update(player, false, nullptr);
        if (triggers.panels()[301].enabled) {
            trap_fired = true;
            break;
        }
    }

    CHECK(trap_fired);
}

TEST_CASE("doom2d map08 Press expander closes floor traps on touch", "[sim][integration]") {
    const auto map_path =
        std::filesystem::path(D2DF_SOURCE_DIR) / "assets/content/maps/doom2d/map08.json";
    if (!std::filesystem::exists(map_path)) {
        SKIP("doom2d map08 not available");
    }

    const auto doc = map::load_map_json_v1(map_path);
    sim::TriggerSystem triggers;
    triggers.reset(doc);

    CHECK_FALSE(triggers.panels()[270].enabled);
    CHECK_FALSE(triggers.panels()[275].enabled);

    sim::PlayerState player;
    player.snap_to(983.0f, 204.0f);
    triggers.update(player, false);

    CHECK(triggers.panels()[270].enabled);
    CHECK(triggers.panels()[275].enabled);
}

TEST_CASE("Press expander activates overlapping trigger after wait", "[sim]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& door = doc.panels.back();
    door.type = map::PANEL_CLOSEDOOR;
    door.position = {100, 100};
    door.size = {16, 64};

    doc.triggers.push_back(map::MapTrigger{});
    auto& open = doc.triggers.back();
    open.type = map::TriggerType::OpenDoor;
    open.target_panel = 0;
    open.position = {90, 120};
    open.size = {16, 16};
    open.enabled = true;

    doc.triggers.push_back(map::MapTrigger{});
    auto& press = doc.triggers.back();
    press.type = map::TriggerType::Press;
    press.position = {80, 80};
    press.size = {64, 64};
    press.activate = map::ActivateType::None;

    sim::TriggerSystem triggers;
    triggers.reset(doc);

    sim::PlayerState player;
    triggers.update(player, false);

    CHECK(triggers.panels()[0].enabled == false);
}

TEST_CASE("ACTIVATE_NOMONSTER Press expander fires without monsters", "[sim]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& door = doc.panels.back();
    door.type = map::PANEL_CLOSEDOOR;
    door.position = {200, 100};
    door.size = {16, 64};

    doc.triggers.push_back(map::MapTrigger{});
    auto& open = doc.triggers.back();
    open.type = map::TriggerType::OpenDoor;
    open.target_panel = 0;
    open.position = {210, 120};
    open.size = {16, 16};
    open.enabled = true;

    doc.triggers.push_back(map::MapTrigger{});
    auto& press = doc.triggers.back();
    press.type = map::TriggerType::Press;
    press.position = {180, 80};
    press.size = {80, 80};
    press.activate = map::ActivateType::NoMonster;

    sim::TriggerSystem triggers;
    triggers.reset(doc);

    sim::PlayerState player;
    triggers.update(player, false);

    CHECK_FALSE(triggers.panels()[0].enabled);
}

TEST_CASE("MapCollision trace_solid_ray hits wall panel", "[sim][combat]") {
    map::MapDocument doc;
    doc.panels.push_back(map::MapPanel{});
    auto& wall = doc.panels.back();
    wall.type = map::PANEL_WALL;
    wall.position = {200, 100};
    wall.size = {32, 32};

    sim::MapCollision collision;
    collision.build_from_map(doc);

    const auto hit = collision.trace_solid_ray(50.0f, 116.0f, 300.0f, 116.0f);
    REQUIRE(hit.hit);
    CHECK(hit.x <= 200.0f + 0.5f);
    CHECK(hit.x >= 199.0f);
}

TEST_CASE("pistol hitscan damages shootable target", "[sim][combat]") {
    map::MapDocument doc;
    doc.size = {512, 512};

    sim::MapCollision collision;
    collision.build_from_map(doc);

    sim::PlayerState player;
    player.snap_to(100.0f, 100.0f);
    player.combat().facing_left = false;
    player.combat().select_weapon(sim::WeaponId::Pistol);
    give_ammo(player.combat(), sim::AmmoType::Bullets, 50);
    REQUIRE(player.combat().has_ammo_for_weapon(sim::WeaponId::Pistol));

    std::vector<sim::ShootableTarget> targets;
    sim::ShootableTarget target;
    target.id = 100;
    target.x = 200.0f;
    target.y = 100.0f;
    target.width = 34.0f;
    target.height = 52.0f;
    target.health = 100;
    targets.push_back(target);

    sim::WeaponSystem weapons;
    sim::PlayerInput fire_input{};
    fire_input.fire = true;

    weapons.tick(player, collision, fire_input, targets, nullptr, nullptr, 1, nullptr,
                 doc.size.width);

    CHECK(targets[0].health < 100);
    CHECK(player.combat().ammo_for_current_weapon() == 49);
}

TEST_CASE("PlayerFired and EntityDamaged events on pistol fire", "[sim][combat]") {
    map::MapDocument doc;
    doc.size = {512, 512};

    sim::MapCollision collision;
    collision.build_from_map(doc);

    sim::PlayerState player;
    player.snap_to(100.0f, 100.0f);
    player.combat().facing_left = false;
    player.combat().select_weapon(sim::WeaponId::Pistol);
    give_ammo(player.combat(), sim::AmmoType::Bullets, 50);
    REQUIRE(player.combat().has_ammo_for_weapon(sim::WeaponId::Pistol));

    std::vector<sim::ShootableTarget> targets;
    sim::ShootableTarget target;
    target.id = 200;
    target.x = 200.0f;
    target.y = 100.0f;
    target.width = 34.0f;
    target.height = 52.0f;
    targets.push_back(target);

    d2df::EventBus bus;
    int fired = 0;
    int damaged = 0;
    bus.subscribe<events::PlayerFired>([&](const events::PlayerFired&) { ++fired; });
    bus.subscribe<events::EntityDamaged>([&](const events::EntityDamaged& e) {
        if (e.target_id == 200) {
            ++damaged;
        }
    });

    sim::WeaponSystem weapons;
    sim::PlayerInput fire_input{};
    fire_input.fire = true;
    weapons.tick(player, collision, fire_input, targets, nullptr, nullptr, 1, &bus,
                 doc.size.width);

    CHECK(fired == 1);
    CHECK(damaged == 1);
}

TEST_CASE("shotgun hitscan damages shootable target", "[sim][combat]") {
    map::MapDocument doc;
    doc.size = {512, 512};

    sim::MapCollision collision;
    collision.build_from_map(doc);

    sim::PlayerState player;
    player.snap_to(100.0f, 100.0f);
    player.combat().facing_left = false;
    own_weapon(player.combat(), sim::WeaponId::Shotgun1);
    player.combat().select_weapon(sim::WeaponId::Shotgun1);
    give_ammo(player.combat(), sim::AmmoType::Shells, 20);

    std::vector<sim::ShootableTarget> targets;
    sim::ShootableTarget target;
    target.id = 101;
    target.x = 200.0f;
    target.y = 100.0f;
    target.width = 34.0f;
    target.height = 52.0f;
    target.health = 100;
    targets.push_back(target);

    sim::WeaponSystem weapons;
    sim::PlayerInput fire_input{};
    fire_input.fire = true;

    weapons.tick(player, collision, fire_input, targets, nullptr, nullptr, 1, nullptr,
                 doc.size.width);

    CHECK(targets[0].health < 100);
}

TEST_CASE("ACTIVATE_SHOT trigger opens door on hitscan line", "[sim][combat]") {
    map::MapDocument doc;
    doc.size = {512, 512};

    doc.panels.push_back(map::MapPanel{});
    auto& door = doc.panels.back();
    door.type = map::PANEL_CLOSEDOOR;
    door.position = {300, 100};
    door.size = {32, 64};
    door.enabled = true;

    doc.triggers.push_back(map::MapTrigger{});
    auto& shot = doc.triggers.back();
    shot.type = map::TriggerType::OpenDoor;
    shot.target_panel = 0;
    shot.position = {250, 120};
    shot.size = {32, 32};
    shot.enabled = true;
    shot.activate = map::ActivateType::Shot;

    sim::TriggerSystem triggers;
    triggers.reset(doc);

    sim::MapCollision collision;
    collision.build_from_panels(triggers.panels());

    sim::PlayerState player;
    player.snap_to(100.0f, 100.0f);
    player.combat().facing_left = false;
    player.combat().select_weapon(sim::WeaponId::Pistol);
    give_ammo(player.combat(), sim::AmmoType::Bullets, 50);
    REQUIRE(player.combat().has_ammo_for_weapon(sim::WeaponId::Pistol));

    sim::WeaponSystem weapons;
    sim::PlayerInput fire_input{};
    fire_input.fire = true;
    std::vector<sim::ShootableTarget> no_targets;

    weapons.tick(player, collision, fire_input, no_targets, &triggers, nullptr, 1, nullptr,
                 doc.size.width);

    CHECK_FALSE(triggers.panels()[0].enabled);
}

TEST_CASE("rocket projectile stops at wall panel", "[sim][combat]") {
    map::MapDocument doc;
    doc.size = {512, 512};
    doc.panels.push_back(map::MapPanel{});
    auto& wall = doc.panels.back();
    wall.type = map::PANEL_WALL;
    wall.position = {200, 100};
    wall.size = {32, 64};

    sim::MapCollision collision;
    collision.build_from_map(doc);

    sim::ProjectileSystem projectiles;
    projectiles.spawn_rocket(120.0f, 116.0f, 400.0f, 116.0f, 1);

    sim::PlayerState player;
    std::vector<sim::ShootableTarget> no_targets;
    float max_x_seen = 0.0f;
    for (int i = 0; i < 40; ++i) {
        for (const auto& projectile : projectiles.projectiles()) {
            if (projectile.active) {
                max_x_seen = std::max(max_x_seen, projectile.x);
            }
        }
        projectiles.tick(collision, nullptr, player, no_targets, nullptr, doc.size.width,
                         doc.size.height);
    }

    const bool rocket_gone =
        projectiles.projectiles().empty() || !projectiles.projectiles()[0].active;
    CHECK(rocket_gone);
    CHECK(max_x_seen < 200.0f);
}

TEST_CASE("BFG charge spawns projectile after warmup", "[sim][combat]") {
    map::MapDocument doc;
    doc.size = {512, 512};

    sim::MapCollision collision;
    collision.build_from_map(doc);

    sim::PlayerState player;
    player.snap_to(100.0f, 100.0f);
    own_weapon(player.combat(), sim::WeaponId::Bfg);
    player.combat().select_weapon(sim::WeaponId::Bfg);
    player.combat().ammo[static_cast<std::size_t>(sim::AmmoType::Cells)] = 80;

    sim::ProjectileSystem projectiles;
    sim::WeaponSystem weapons;
    sim::PlayerInput fire_input{};
    fire_input.fire = true;
    std::vector<sim::ShootableTarget> no_targets;

    weapons.tick(player, collision, fire_input, no_targets, nullptr, &projectiles, 1, nullptr,
                 doc.size.width);
    CHECK(player.combat().bfg_charge_ticks == 17);
    CHECK(player.combat().ammo[static_cast<std::size_t>(sim::AmmoType::Cells)] == 40);
    CHECK(projectiles.projectiles().empty());

    sim::PlayerInput idle{};
    for (int i = 0; i < 18; ++i) {
        weapons.tick(player, collision, idle, no_targets, nullptr, &projectiles, 1, nullptr,
                     doc.size.width);
    }

    bool bfg_active = false;
    for (const auto& projectile : projectiles.projectiles()) {
        if (projectile.active && projectile.weapon == sim::WeaponId::Bfg) {
            bfg_active = true;
        }
    }
    CHECK(bfg_active);
    CHECK(player.combat().bfg_charge_ticks == -1);
}

TEST_CASE("super chaingun hitscan damages shootable target", "[sim][combat]") {
    map::MapDocument doc;
    doc.size = {512, 512};

    sim::MapCollision collision;
    collision.build_from_map(doc);

    sim::PlayerState player;
    player.snap_to(100.0f, 100.0f);
    player.combat().facing_left = false;
    own_weapon(player.combat(), sim::WeaponId::SuperChaingun);
    player.combat().select_weapon(sim::WeaponId::SuperChaingun);
    give_ammo(player.combat(), sim::AmmoType::Shells, 10);

    std::vector<sim::ShootableTarget> targets;
    targets.push_back(sim::ShootableTarget{101, map::MonsterType::None, 200.0f, 108.0f, 200.0f,
                                           108.0f, 32.0f, 32.0f, 0, 0, 100, 100, 0, false});

    sim::WeaponSystem weapons;
    sim::PlayerInput fire_input{};
    fire_input.fire = true;

    weapons.tick(player, collision, fire_input, targets, nullptr, nullptr, 1, nullptr,
                 doc.size.width);

    CHECK(targets[0].health < 100);
    CHECK(player.combat().ammo[static_cast<std::size_t>(sim::AmmoType::Shells)] == 9);
}

TEST_CASE("medkit pickup restores player health", "[sim][items]") {
    map::MapDocument doc;
    map::MapItem medkit;
    medkit.position = {100, 100};
    medkit.type = map::ItemType::MedkitSmall;
    doc.items.push_back(medkit);

    sim::ItemSystem items;
    items.reset(doc);

    sim::PlayerState player;
    player.snap_to(100.0f, 100.0f);
    player.apply_damage(30);

    items.tick(player, nullptr, nullptr);

    CHECK(player.health() == 80);
    CHECK_FALSE(items.items()[0].active);
}

TEST_CASE("ammo pickup increases combat ammo", "[sim][items]") {
    map::MapDocument doc;
    map::MapItem shells;
    shells.position = {100, 100};
    shells.type = map::ItemType::AmmoShells;
    doc.items.push_back(shells);

    sim::ItemSystem items;
    items.reset(doc);

    sim::PlayerState player;
    player.snap_to(100.0f, 100.0f);

    items.tick(player, nullptr, nullptr);

    CHECK(player.combat().ammo[static_cast<std::size_t>(sim::AmmoType::Shells)] == 4);
    CHECK_FALSE(items.items()[0].active);
}

TEST_CASE("armor pickup raises player armor", "[sim][items]") {
    map::MapDocument doc;
    map::MapItem armor;
    armor.position = {100, 100};
    armor.type = map::ItemType::ArmorGreen;
    doc.items.push_back(armor);

    sim::ItemSystem items;
    items.reset(doc);

    sim::PlayerState player;
    player.snap_to(100.0f, 100.0f);

    items.tick(player, nullptr, nullptr);

    CHECK(player.armor() == sim::PlayerState::kArmorSoftCap);
    CHECK_FALSE(items.items()[0].active);
}

TEST_CASE("key pickup grants inventory key", "[sim][items]") {
    map::MapDocument doc;
    map::MapItem key;
    key.position = {100, 100};
    key.type = map::ItemType::KeyRed;
    doc.items.push_back(key);

    sim::ItemSystem items;
    items.reset(doc);

    sim::PlayerState player;
    player.snap_to(100.0f, 100.0f);

    items.tick(player, nullptr, nullptr);

    CHECK(player.has_key_red());
    CHECK_FALSE(items.items()[0].active);
}

TEST_CASE("ammo item respawns after pickup in single player", "[sim][items]") {
    map::MapDocument doc;
    map::MapItem shells;
    shells.position = {100, 100};
    shells.type = map::ItemType::AmmoShells;
    doc.items.push_back(shells);

    sim::ItemSystem items;
    items.reset(doc);

    sim::PlayerState player;
    player.snap_to(100.0f, 100.0f);

    items.tick(player, nullptr, nullptr);

    CHECK_FALSE(items.items()[0].active);
    CHECK(items.items()[0].respawnable);
    CHECK(items.items()[0].respawn_countdown == sim::kDefaultItemRespawnTicks);

    player.snap_to(0.0f, 0.0f);

    for (int i = 0; i < sim::kDefaultItemRespawnTicks - 1; ++i) {
        items.tick(player, nullptr, nullptr);
        CHECK_FALSE(items.items()[0].active);
    }

    items.tick(player, nullptr, nullptr);
    CHECK(items.items()[0].active);
}

TEST_CASE("armor absorbs damage before health", "[sim][items]") {
    sim::PlayerState player;
    player.snap_to(0.0f, 0.0f);
    player.set_armor(50);

    const bool died = player.apply_damage(30);
    CHECK_FALSE(died);
    CHECK(player.armor() == 20);
    CHECK(player.health() == sim::PlayerState::kMaxHealth);

    player.apply_damage(50);
    CHECK(player.armor() == 0);
    CHECK(player.health() == 70);
}

TEST_CASE("key-gated door requires matching key", "[sim][triggers]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& door = doc.panels.back();
    door.type = map::PANEL_CLOSEDOOR;
    door.position = {100, 100};
    door.size = {16, 48};
    door.enabled = true;

    doc.triggers.push_back(map::MapTrigger{});
    auto& trigger = doc.triggers.back();
    trigger.type = map::TriggerType::OpenDoor;
    trigger.position = {80, 120};
    trigger.size = {32, 32};
    trigger.target_panel = 0;
    trigger.activate = map::ActivateType::PlayerPress;
    trigger.keys = map::KeyGreen;

    sim::TriggerSystem triggers;
    triggers.reset(doc);

    sim::PlayerState player;
    player.snap_to(90.0f, 120.0f);

    triggers.update(player, true);
    CHECK(triggers.panels()[0].enabled);

    player.give_key_green();
    triggers.update(player, true);
    CHECK_FALSE(triggers.panels()[0].enabled);
}

TEST_CASE("invul powerup blocks damage", "[sim][items]") {
    sim::PlayerState player;
    player.snap_to(0.0f, 0.0f);
    player.give_invul();

    const bool died = player.apply_damage(50);
    CHECK_FALSE(died);
    CHECK(player.health() == sim::PlayerState::kMaxHealth);
}

TEST_CASE("suit powerup blocks acid damage tick", "[sim][items]") {
    map::MapDocument doc;
    doc.panels.push_back(map::MapPanel{});
    auto& acid = doc.panels.back();
    acid.type = map::PANEL_ACID1;
    acid.position = {0, 100};
    acid.size = {200, 16};

    sim::MapCollision collision;
    collision.build_from_panels(doc.panels);

    ecs::GameWorld world;
    world.reset_player(doc);
    auto& world_player = world.player();
    world_player.snap_to(50.0f, 80.0f);
    world_player.give_suit();

    d2df::EventBus events;
    for (int i = 0; i < sim::PlayerState::kAcidDamagePeriod * 2; ++i) {
        world.fixed_update(collision, {}, &events, doc.size.width, doc.size.height, nullptr);
    }
    CHECK(world_player.health() == sim::PlayerState::kMaxHealth);
}

TEST_CASE("coop shared keys stay on map after pickup", "[sim][items]") {
    map::MapDocument doc;
    map::MapItem key;
    key.position = {100, 100};
    key.type = map::ItemType::KeyGreen;
    doc.items.push_back(key);

    sim::GameRules rules;
    rules.mode = sim::GameMode::Coop;

    sim::ItemSystem items;
    items.reset(doc, rules);

    sim::PlayerState player;
    player.snap_to(100.0f, 100.0f);

    items.tick(player, nullptr, nullptr);

    CHECK(player.has_key_green());
    CHECK(items.items()[0].active);
}

TEST_CASE("weapons stay keeps owned weapon on map", "[sim][items]") {
    map::MapDocument doc;
    map::MapItem shotgun;
    shotgun.position = {100, 100};
    shotgun.type = map::ItemType::WeaponShotgun1;
    doc.items.push_back(shotgun);

    sim::GameRules rules;
    rules.mode = sim::GameMode::Deathmatch;
    rules.weapons_stay = true;

    sim::ItemSystem items;
    items.reset(doc, rules);

    sim::PlayerState player;
    player.snap_to(100.0f, 100.0f);

    items.tick(player, nullptr, nullptr);
    CHECK(items.items()[0].active);

    items.tick(player, nullptr, nullptr);
    CHECK(player.combat().owned[static_cast<std::size_t>(sim::WeaponId::Shotgun1)]);
}

TEST_CASE("deathmatch items respawn after pickup", "[sim][items]") {
    map::MapDocument doc;
    map::MapItem invul;
    invul.position = {100, 100};
    invul.type = map::ItemType::Invul;
    doc.items.push_back(invul);

    sim::GameRules rules;
    rules.mode = sim::GameMode::Deathmatch;

    sim::ItemSystem items;
    items.reset(doc, rules);

    sim::PlayerState player;
    player.snap_to(100.0f, 100.0f);

    items.tick(player, nullptr, nullptr);
    CHECK_FALSE(items.items()[0].active);
    CHECK(items.items()[0].respawnable);

    player.snap_to(0.0f, 0.0f);
    for (int i = 0; i < sim::kDefaultItemRespawnTicks; ++i) {
        items.tick(player, nullptr, nullptr);
    }
    CHECK(items.items()[0].active);
    CHECK(items.items()[0].respawn_anim_tick > 0);
}

TEST_CASE("falling item moves downward with gravity", "[sim][items]") {
    map::MapDocument doc;
    doc.panels.push_back(map::MapPanel{});
    auto& floor = doc.panels.back();
    floor.type = map::PANEL_WALL;
    floor.position = {0, 200};
    floor.size = {256, 16};

    map::MapItem key;
    key.position = {100, 50};
    key.type = map::ItemType::KeyRed;
    key.fall = true;
    doc.items.push_back(key);

    sim::ItemSystem items;
    items.reset(doc);

    sim::MapCollision collision;
    collision.build_from_panels(doc.panels);

    sim::PlayerState player;
    player.snap_to(0.0f, 0.0f);

    const float start_y = items.items()[0].y;
    for (int i = 0; i < 120; ++i) {
        items.tick(player, &collision, nullptr);
    }
    CHECK(items.items()[0].y > start_y);
}

TEST_CASE("map monsters load from JSON", "[sim][monsters]") {
    const auto map_path =
        std::filesystem::path(D2DF_SOURCE_DIR) / "assets/content/maps/doom2d/map08.json";
    if (!std::filesystem::exists(map_path)) {
        SKIP("map08.json not available");
    }

    const auto doc = map::load_map_json_v1(map_path);
    CHECK_FALSE(doc.monsters.empty());

    ecs::GameWorld world;
    world.reset_player(doc);
    CHECK(world.targets().size() == doc.monsters.size());
    CHECK(world.targets().front().is_monster());
}

TEST_CASE("spawn loadout includes pistol with 50 bullets", "[sim][player]") {
    sim::PlayerState player;
    player.reset_to_spawn(0.0f, 0.0f);

    CHECK(player.combat().owned[static_cast<std::size_t>(sim::WeaponId::Pistol)]);
    CHECK(player.combat().ammo[static_cast<std::size_t>(sim::AmmoType::Bullets)] == 50);
}

TEST_CASE("teleport preserves weapons and ammo", "[sim][triggers]") {
    map::MapDocument doc;

    doc.triggers.push_back(map::MapTrigger{});
    auto& trigger = doc.triggers.back();
    trigger.type = map::TriggerType::Teleport;
    trigger.position = {0, 0};
    trigger.size = {32, 32};
    trigger.activate = map::ActivateType::PlayerCollide;
    trigger.teleport_target = {200, 100};
    trigger.d2d = true;

    sim::TriggerSystem triggers;
    triggers.reset(doc);

    sim::MapCollision collision;
    collision.build_from_panels(triggers.panels());

    sim::PlayerState player;
    player.reset_to_spawn(4.0f, 4.0f);
    own_weapon(player.combat(), sim::WeaponId::Chaingun);
    give_ammo(player.combat(), sim::AmmoType::Bullets, 100);
    player.combat().select_weapon(sim::WeaponId::Chaingun);

    triggers.update(player, false, nullptr, nullptr, &collision);

    CHECK(player.combat().owned[static_cast<std::size_t>(sim::WeaponId::Chaingun)]);
    CHECK(player.combat().ammo[static_cast<std::size_t>(sim::AmmoType::Bullets)] == 100);
    CHECK(player.combat().current_weapon == sim::WeaponId::Chaingun);
    CHECK(std::abs(player.x - (200.0f - sim::PlayerState::width * 0.5f)) < 0.01f);
}

TEST_CASE("blocked teleport keeps player position", "[sim][triggers]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& wall = doc.panels.back();
    wall.type = map::PANEL_WALL;
    wall.position = {196, 48};
    wall.size = {32, 64};

    doc.triggers.push_back(map::MapTrigger{});
    auto& trigger = doc.triggers.back();
    trigger.type = map::TriggerType::Teleport;
    trigger.position = {0, 0};
    trigger.size = {32, 32};
    trigger.activate = map::ActivateType::PlayerCollide;
    trigger.teleport_target = {200, 100};
    trigger.d2d = true;

    sim::TriggerSystem triggers;
    triggers.reset(doc);

    sim::MapCollision collision;
    collision.build_from_panels(triggers.panels());

    sim::PlayerState player;
    player.snap_to(4.0f, 4.0f);
    const float start_x = player.x;

    d2df::EventBus events;
    triggers.update(player, false, &events, nullptr, &collision);

    CHECK(player.x == start_x);
}

TEST_CASE("jetpack activates in air when jump pressed", "[sim][player]") {
    map::MapDocument doc;
    doc.panels.push_back(map::MapPanel{});
    auto& floor = doc.panels.back();
    floor.type = map::PANEL_WALL;
    floor.position = {0, 200};
    floor.size = {256, 16};

    sim::MapCollision collision;
    collision.build_from_panels(doc.panels);

    sim::PlayerState player;
    player.snap_to(100.0f, 100.0f);
    player.refill_jetpack();

    sim::PlayerInput none{};
    for (int i = 0; i < 4; ++i) {
        player.fixed_update(collision, none);
    }

    sim::PlayerInput jump{};
    jump.jump = true;
    player.fixed_update(collision, jump);

    CHECK(player.jetpack_active());
    CHECK(player.jet_fuel() < sim::PlayerState::kJetFuelMax);
}

TEST_CASE("monster chases and damages nearby player", "[sim][monsters]") {
    map::MapDocument doc;
    doc.panels.push_back(map::MapPanel{});
    auto& floor = doc.panels.back();
    floor.type = map::PANEL_WALL;
    floor.position = {0, 200};
    floor.size = {512, 16};

    sim::MapCollision collision;
    collision.build_from_panels(doc.panels);

    sim::PlayerState player;
    player.snap_to(200.0f, 140.0f);

    std::vector<sim::ShootableTarget> targets;
    sim::ShootableTarget monster;
    monster.id = 200;
    monster.monster_type = map::MonsterType::Zomby;
    monster.x = 120.0f;
    monster.y = 140.0f;
    monster.prev_x = monster.x;
    monster.prev_y = monster.y;
    monster.width = 34.0f;
    monster.height = 52.0f;
    monster.max_health = 15;
    monster.health = 15;
    targets.push_back(monster);

    sim::MonsterSystem monsters;
    const int health_before = player.health();
    for (int i = 0; i < 120; ++i) {
        monsters.tick(collision, player, targets, nullptr);
    }

    CHECK(targets[0].x > 120.0f);
    CHECK(player.health() < health_before);
}

TEST_CASE("monster does not chase player hidden behind wall", "[sim][monsters]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& floor = doc.panels.back();
    floor.type = map::PANEL_WALL;
    floor.position = {0, 200};
    floor.size = {512, 16};

    doc.panels.push_back(map::MapPanel{});
    auto& wall = doc.panels.back();
    wall.type = map::PANEL_WALL;
    wall.position = {160, 120};
    wall.size = {16, 80};

    sim::MapCollision collision;
    collision.build_from_panels(doc.panels);

    sim::PlayerState player;
    player.snap_to(220.0f, 140.0f);

    std::vector<sim::ShootableTarget> targets;
    sim::ShootableTarget monster;
    monster.id = 200;
    monster.monster_type = map::MonsterType::Zomby;
    monster.x = 100.0f;
    monster.y = 140.0f;
    monster.prev_x = monster.x;
    monster.prev_y = monster.y;
    monster.width = 34.0f;
    monster.height = 52.0f;
    monster.max_health = 15;
    monster.health = 15;
    monster.facing_left = false;
    targets.push_back(monster);

    sim::MonsterSystem monsters;
    for (int i = 0; i < 120; ++i) {
        monsters.tick(collision, player, targets, nullptr);
    }

    CHECK(targets[0].x == 100.0f);
}

TEST_CASE("damaged monster chases player even through wall", "[sim][monsters]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& floor = doc.panels.back();
    floor.type = map::PANEL_WALL;
    floor.position = {0, 200};
    floor.size = {512, 16};

    doc.panels.push_back(map::MapPanel{});
    auto& wall = doc.panels.back();
    wall.type = map::PANEL_WALL;
    wall.position = {160, 120};
    wall.size = {16, 80};

    sim::MapCollision collision;
    collision.build_from_panels(doc.panels);

    sim::PlayerState player;
    player.snap_to(220.0f, 140.0f);

    std::vector<sim::ShootableTarget> targets;
    sim::ShootableTarget monster;
    monster.id = 200;
    monster.monster_type = map::MonsterType::Zomby;
    monster.x = 100.0f;
    monster.y = 140.0f;
    monster.prev_x = monster.x;
    monster.prev_y = monster.y;
    monster.width = 34.0f;
    monster.height = 52.0f;
    monster.max_health = 15;
    monster.health = 15;
    monster.facing_left = false;
    targets.push_back(monster);
    targets[0].apply_damage(5, d2df::kPlayerEntityId);

    sim::MonsterSystem monsters;
    for (int i = 0; i < 120; ++i) {
        monsters.tick(collision, player, targets, nullptr);
    }

    CHECK(targets[0].x > 100.0f);
}

TEST_CASE("flying soul keeps altitude without ground physics", "[sim][monsters]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& floor = doc.panels.back();
    floor.type = map::PANEL_WALL;
    floor.position = {0, 400};
    floor.size = {512, 16};

    sim::MapCollision collision;
    collision.build_from_panels(doc.panels);

    sim::PlayerState player;
    player.snap_to(300.0f, 500.0f);

    std::vector<sim::ShootableTarget> targets;
    sim::ShootableTarget soul;
    soul.id = 200;
    soul.monster_type = map::MonsterType::Soul;
    soul.x = 200.0f;
    soul.y = 180.0f;
    soul.prev_x = soul.x;
    soul.prev_y = soul.y;
    soul.width = 32.0f;
    soul.height = 36.0f;
    soul.max_health = 60;
    soul.health = 60;
    targets.push_back(soul);

    sim::MonsterSystem monsters;
    for (int i = 0; i < 120; ++i) {
        monsters.tick(collision, player, targets, nullptr);
    }

    CHECK(targets[0].y < 250.0f);
}

TEST_CASE("falling item rises inside lift-up zone", "[sim][items]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& floor = doc.panels.back();
    floor.type = map::PANEL_WALL;
    floor.position = {0, 200};
    floor.size = {256, 16};

    doc.panels.push_back(map::MapPanel{});
    auto& lift = doc.panels.back();
    lift.type = map::PANEL_LIFTUP;
    lift.position = {80, 120};
    lift.size = {64, 64};

    map::MapItem key;
    key.position = {100, 140};
    key.type = map::ItemType::KeyRed;
    key.fall = true;
    doc.items.push_back(key);

    sim::ItemSystem items;
    items.reset(doc);

    sim::MapCollision collision;
    collision.build_from_panels(doc.panels);

    sim::PlayerState player;
    player.snap_to(0.0f, 0.0f);

    const float start_y = items.items()[0].y;
    for (int i = 0; i < 120; ++i) {
        items.tick(player, &collision, nullptr);
    }
    CHECK(items.items()[0].y < start_y);
}

TEST_CASE("ground monster rises inside lift-up zone", "[sim][monsters]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& floor = doc.panels.back();
    floor.type = map::PANEL_WALL;
    floor.position = {0, 200};
    floor.size = {256, 16};

    doc.panels.push_back(map::MapPanel{});
    auto& lift = doc.panels.back();
    lift.type = map::PANEL_LIFTUP;
    lift.position = {80, 120};
    lift.size = {64, 64};

    sim::MapCollision collision;
    collision.build_from_panels(doc.panels);

    sim::PlayerState player;
    player.snap_to(0.0f, 0.0f);

    std::vector<sim::ShootableTarget> targets;
    sim::ShootableTarget zomby;
    zomby.id = 200;
    zomby.monster_type = map::MonsterType::Zomby;
    zomby.x = 100.0f;
    zomby.y = 140.0f;
    zomby.prev_x = zomby.x;
    zomby.prev_y = zomby.y;
    zomby.width = 34.0f;
    zomby.height = 52.0f;
    zomby.max_health = 20;
    zomby.health = 20;
    targets.push_back(zomby);

    sim::MonsterSystem monsters;
    const float start_y = targets[0].y;
    for (int i = 0; i < 120; ++i) {
        monsters.tick(collision, player, targets, nullptr);
    }
    CHECK(targets[0].y < start_y);
}

TEST_CASE("monster takes periodic acid damage", "[sim][monsters]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& acid = doc.panels.back();
    acid.type = map::PANEL_ACID1;
    acid.position = {0, 0};
    acid.size = {128, 128};

    sim::MapCollision collision;
    collision.build_from_panels(doc.panels);

    sim::PlayerState player;
    player.snap_to(500.0f, 500.0f);

    std::vector<sim::ShootableTarget> targets;
    sim::ShootableTarget zomby;
    zomby.id = 200;
    zomby.monster_type = map::MonsterType::Zomby;
    zomby.x = 32.0f;
    zomby.y = 32.0f;
    zomby.prev_x = zomby.x;
    zomby.prev_y = zomby.y;
    zomby.width = 34.0f;
    zomby.height = 52.0f;
    zomby.max_health = 100;
    zomby.health = 100;
    targets.push_back(zomby);

    sim::MonsterSystem monsters;
    sim::PlayerInput none{};
    for (int i = 0; i < 180; ++i) {
        player.fixed_update(collision, none);
        monsters.tick(collision, player, targets, nullptr);
    }
    CHECK(targets[0].health < 100);
}

TEST_CASE("doom2d map06 lift-down trigger flips shaft direction", "[sim][integration]") {
    const auto map_path =
        std::filesystem::path(D2DF_SOURCE_DIR) / "assets/content/maps/doom2d/map06.json";
    if (!std::filesystem::exists(map_path)) {
        SKIP("map06.json not available");
    }

    const auto doc = map::load_map_json_v1(map_path);
    sim::TriggerSystem triggers;
    triggers.reset(doc);

    CHECK((triggers.panels()[450].type & map::PANEL_LIFTUP) != 0);

    sim::PlayerState player;
    player.snap_to(1300.0f, 1310.0f);
    triggers.update(player, true);

    CHECK((triggers.panels()[450].type & map::PANEL_LIFTDOWN) != 0);
}

TEST_CASE("dead soul is removed from target list", "[sim][monsters]") {
    ecs::GameWorld world;
    map::MapDocument doc;
    doc.panels.push_back(map::MapPanel{});
    auto& floor = doc.panels.back();
    floor.type = map::PANEL_WALL;
    floor.position = {0, 400};
    floor.size = {512, 16};

    doc.monsters.push_back(map::MapMonster{{200, 180}, map::MonsterType::Soul});
    world.reset_player(doc);
    CHECK(world.targets().size() == 1);

    world.targets().front().apply_damage(100, d2df::kPlayerEntityId);
    CHECK_FALSE(world.targets().front().alive());

    sim::MapCollision collision;
    collision.build_from_panels(doc.panels);
    sim::PlayerInput none{};
    for (int i = 0; i < 4; ++i) {
        world.fixed_update(collision, none, nullptr, 512, 512, nullptr);
    }
    CHECK(world.targets().empty());
}

TEST_CASE("imp shoots player at range with line of sight", "[sim][monsters]") {
    map::MapDocument doc;
    doc.panels.push_back(map::MapPanel{});
    auto& floor = doc.panels.back();
    floor.type = map::PANEL_WALL;
    floor.position = {0, 300};
    floor.size = {512, 16};

    sim::MapCollision collision;
    collision.build_from_panels(doc.panels);

    sim::PlayerState player;
    player.snap_to(320.0f, 220.0f);

    std::vector<sim::ShootableTarget> targets;
    sim::ShootableTarget imp;
    imp.id = 200;
    imp.monster_type = map::MonsterType::Imp;
    imp.x = 80.0f;
    imp.y = 220.0f;
    imp.prev_x = imp.x;
    imp.prev_y = imp.y;
    imp.width = 34.0f;
    imp.height = 50.0f;
    imp.max_health = 25;
    imp.health = 25;
    imp.facing_left = false;
    targets.push_back(imp);

    sim::MonsterSystem monsters;
    sim::ProjectileSystem projectiles;
    sim::TriggerSystem triggers;
    const int health_before = player.health();
    for (int i = 0; i < 180; ++i) {
        monsters.tick(collision, player, targets, nullptr, &projectiles);
        projectiles.tick(collision, &triggers, player, targets, nullptr, 512, 512);
    }

    CHECK(player.health() < health_before);
}

TEST_CASE("barrel explosion damages nearby player", "[sim][monsters]") {
    ecs::GameWorld world;
    map::MapDocument doc;

    std::vector<sim::ShootableTarget>& targets = world.targets();
    sim::ShootableTarget barrel;
    barrel.id = 200;
    barrel.monster_type = map::MonsterType::Barrel;
    barrel.x = 200.0f;
    barrel.y = 200.0f;
    barrel.prev_x = barrel.x;
    barrel.prev_y = barrel.y;
    barrel.width = 24.0f;
    barrel.height = 36.0f;
    barrel.max_health = 20;
    barrel.health = 20;
    targets.push_back(barrel);

    world.player().snap_to(200.0f, 200.0f);

    sim::MapCollision collision;
    sim::PlayerInput none{};
    targets.front().apply_damage(20, d2df::kPlayerEntityId);

    const int health_before = world.player().health();
    world.fixed_update(collision, none, nullptr, 512, 512, nullptr);
    CHECK(world.player().health() < health_before);
}

TEST_CASE("pain elemental spawns lost soul at range", "[sim][monsters]") {
    map::MapDocument doc;
    doc.panels.push_back(map::MapPanel{});
    auto& floor = doc.panels.back();
    floor.type = map::PANEL_WALL;
    floor.position = {0, 300};
    floor.size = {512, 16};

    sim::MapCollision collision;
    collision.build_from_panels(doc.panels);

    sim::PlayerState player;
    player.snap_to(320.0f, 220.0f);

    std::vector<sim::ShootableTarget> targets;
    sim::ShootableTarget pain;
    pain.id = 200;
    pain.monster_type = map::MonsterType::Pain;
    pain.x = 80.0f;
    pain.y = 200.0f;
    pain.prev_x = pain.x;
    pain.prev_y = pain.y;
    pain.width = 60.0f;
    pain.height = 56.0f;
    pain.max_health = 100;
    pain.health = 100;
    pain.facing_left = false;
    targets.push_back(pain);

    sim::MonsterSystem monsters;
    d2df::EntityId next_id = 300;
    for (int i = 0; i < 120; ++i) {
        monsters.tick(collision, player, targets, nullptr, nullptr, &next_id);
    }

    CHECK(targets.size() >= 2);
    CHECK(std::any_of(targets.begin(), targets.end(), [](const sim::ShootableTarget& target) {
        return target.monster_type == map::MonsterType::Soul && target.alive();
    }));
}

TEST_CASE("pain elemental death spawns three lost souls", "[sim][monsters]") {
    ecs::GameWorld world;
    map::MapDocument doc;

    std::vector<sim::ShootableTarget>& targets = world.targets();
    sim::ShootableTarget pain;
    pain.id = 200;
    pain.monster_type = map::MonsterType::Pain;
    pain.x = 200.0f;
    pain.y = 200.0f;
    pain.prev_x = pain.x;
    pain.prev_y = pain.y;
    pain.width = 60.0f;
    pain.height = 56.0f;
    pain.max_health = 100;
    pain.health = 100;
    targets.push_back(pain);

    world.player().snap_to(500.0f, 500.0f);

    sim::MapCollision collision;
    sim::PlayerInput none{};
    targets.front().apply_damage(100, d2df::kPlayerEntityId);

    world.fixed_update(collision, none, nullptr, 512, 512, nullptr);

    const int soul_count = static_cast<int>(std::count_if(
        targets.begin(), targets.end(), [](const sim::ShootableTarget& target) {
            return target.monster_type == map::MonsterType::Soul && target.alive();
        }));
    CHECK(soul_count == 3);
    CHECK(targets.size() == 3);
}

TEST_CASE("killed zomby becomes corpse instead of immediate removal", "[sim][monsters]") {
    std::vector<sim::ShootableTarget> targets;
    sim::ShootableTarget zomby;
    zomby.id = 200;
    zomby.monster_type = map::MonsterType::Zomby;
    zomby.max_health = 15;
    zomby.health = 15;
    zomby.x = 100.0f;
    zomby.y = 140.0f;
    zomby.prev_x = 80.0f;
    zomby.prev_y = 140.0f;
    zomby.vel_x = 3;
    targets.push_back(zomby);

    targets.front().apply_damage(15, d2df::kPlayerEntityId);
    CHECK_FALSE(targets.front().alive());
    CHECK(targets.front().is_corpse());
    CHECK(std::abs(targets.front().x - 100.0f) < 0.01f);
    CHECK(std::abs(targets.front().prev_x - 100.0f) < 0.01f);
    CHECK(std::abs(targets.front().prev_y - 140.0f) < 0.01f);
    CHECK(targets.front().vel_x == 0);
    CHECK(targets.size() == 1);
}

TEST_CASE("reviving monster returns to life", "[sim][monsters]") {
    std::vector<sim::ShootableTarget> targets;
    sim::ShootableTarget zomby;
    zomby.id = 200;
    zomby.monster_type = map::MonsterType::Zomby;
    zomby.max_health = 15;
    zomby.health = 15;
    targets.push_back(zomby);
    targets.front().apply_damage(15, d2df::kPlayerEntityId);
    targets.front().life_state = sim::MonsterLifeState::Reviving;
    targets.front().revive_ticks = 2;

    sim::MapCollision collision;
    sim::PlayerState player;
    player.snap_to(0.0f, 0.0f);
    sim::MonsterSystem monsters;

    for (int i = 0; i < 3; ++i) {
        monsters.tick(collision, player, targets, nullptr);
    }

    CHECK(targets.front().alive());
    CHECK(targets.front().health == 15);
}

TEST_CASE("arch vile revives overlapping corpse", "[sim][monsters]") {
    map::MapDocument doc;
    sim::MapCollision collision;
    collision.build_from_panels(doc.panels);

    sim::PlayerState player;
    player.snap_to(500.0f, 500.0f);

    std::vector<sim::ShootableTarget> targets;
    sim::ShootableTarget zomby;
    zomby.id = 200;
    zomby.monster_type = map::MonsterType::Zomby;
    zomby.x = 200.0f;
    zomby.y = 200.0f;
    zomby.width = 34.0f;
    zomby.height = 52.0f;
    zomby.max_health = 15;
    zomby.health = 15;
    targets.push_back(zomby);
    targets.front().apply_damage(15, d2df::kPlayerEntityId);

    sim::ShootableTarget vile;
    vile.id = 192;
    vile.monster_type = map::MonsterType::Vile;
    vile.x = 200.0f;
    vile.y = 200.0f;
    vile.width = 68.0f;
    vile.height = 72.0f;
    vile.max_health = 150;
    vile.health = 150;
    targets.push_back(vile);

    sim::MonsterSystem monsters;
    sim::PlayerInput none{};
    bool revived = false;
    for (int i = 0; i < 240; ++i) {
        player.fixed_update(collision, none);
        monsters.tick(collision, player, targets, nullptr);
        if (targets.front().alive()) {
            revived = true;
            break;
        }
    }
    CHECK(revived);
}

TEST_CASE("weapon and monster sfx asset ids resolve", "[sim][audio]") {
    CHECK(std::string_view(sim::weapon_fire_sfx(sim::WeaponId::Pistol)) == "sfx.world.firepistol");
    CHECK(std::string_view(sim::weapon_fire_sfx(sim::WeaponId::RocketLauncher)) ==
          "sfx.world.firerocket");
    CHECK(std::string_view(map::monster_die_sfx(map::MonsterType::Barrel)) ==
          "sfx.monster.barrel_die");
    CHECK(std::string_view(map::item_pickup_sfx(map::ItemType::WeaponChaingun)) ==
          "sfx.world.getweapon");
    CHECK(std::string_view(map::item_pickup_sfx(map::ItemType::MedkitSmall)) ==
          "sfx.world.getitem");
}

TEST_CASE("MonsterDied event publishes on monster death", "[sim][audio]") {
    ecs::GameWorld world;
    map::MapDocument doc;

    std::vector<sim::ShootableTarget>& targets = world.targets();
    sim::ShootableTarget barrel;
    barrel.id = 200;
    barrel.monster_type = map::MonsterType::Barrel;
    barrel.x = 200.0f;
    barrel.y = 200.0f;
    barrel.width = 24.0f;
    barrel.height = 36.0f;
    barrel.max_health = 20;
    barrel.health = 20;
    targets.push_back(barrel);

    world.player().snap_to(500.0f, 500.0f);

    d2df::EventBus bus;
    int died_count = 0;
    bus.subscribe<events::MonsterDied>(
        [&](const events::MonsterDied& event) {
            ++died_count;
            CHECK(event.entity_id == 200);
            CHECK(event.monster_type == static_cast<std::uint8_t>(map::MonsterType::Barrel));
        });

    sim::MapCollision collision;
    sim::PlayerInput none{};
    targets.front().apply_damage(20, d2df::kPlayerEntityId);
    world.fixed_update(collision, none, &bus, 512, 512, nullptr);
    CHECK(died_count == 1);
}

TEST_CASE("Monster sprite placement uses legacy hitbox and draw offsets", "[sim]") {
    const auto zomby_sprite = map::monster_sprite(map::MonsterType::Zomby, false);
    const auto zomby_stats = map::monster_stats(map::MonsterType::Zomby);
    const auto zomby_place = map::monster_sprite_placement(
        map::MonsterType::Zomby, 100.0f, 200.0f, zomby_stats.width, zomby_sprite, false);

    CHECK(std::abs(zomby_place.x - 86.0f) < 0.01f);
    CHECK(std::abs(zomby_place.y - 188.0f) < 0.01f);
    CHECK_FALSE(zomby_place.flip_horizontal);

    const auto soul_sprite = map::monster_sprite(map::MonsterType::Soul, false);
    CHECK(soul_sprite.frame_count == 2);

    const auto soul_stats = map::monster_stats(map::MonsterType::Soul);
    const auto soul_place = map::monster_sprite_placement(
        map::MonsterType::Soul, 100.0f, 200.0f, soul_stats.width, soul_sprite, false);

    CHECK(std::abs(soul_place.x - 85.0f) < 0.01f);
    CHECK(std::abs(soul_place.y - 176.0f) < 0.01f);

    const auto caco_sprite = map::monster_sprite(map::MonsterType::Caco, false);
    CHECK(caco_sprite.frame_count == 1);
}

TEST_CASE("OpenDoor trigger publishes door open event", "[sim][audio]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& door = doc.panels.back();
    door.type = map::PANEL_CLOSEDOOR;
    door.position = {100, 80};
    door.size = {16, 48};

    doc.triggers.push_back(map::MapTrigger{});
    auto& trigger = doc.triggers.back();
    trigger.type = map::TriggerType::OpenDoor;
    trigger.target_panel = 0;
    trigger.position = {80, 80};
    trigger.size = {16, 16};
    trigger.activate = map::ActivateType::PlayerPress;

    sim::TriggerSystem triggers;
    triggers.reset(doc);

    EventBus bus;
    int door_events = 0;
    bus.subscribe<events::WorldDoorChanged>([&](const events::WorldDoorChanged& event) {
        ++door_events;
        CHECK(event.sound == events::DoorSound::Open);
    });

    sim::PlayerState player;
    player.snap_to(78.0f, 78.0f);
    triggers.update(player, true, &bus);

    CHECK(door_events == 1);
    CHECK_FALSE(triggers.panels()[0].enabled);
}

TEST_CASE("Lift toggle publishes switch event when direction changes", "[sim][audio]") {
    map::MapDocument doc;

    doc.panels.push_back(map::MapPanel{});
    auto& lift_zone = doc.panels.back();
    lift_zone.type = map::PANEL_LIFTUP;
    lift_zone.position = {200, 80};
    lift_zone.size = {64, 64};

    doc.triggers.push_back(map::MapTrigger{});
    auto& trigger = doc.triggers.back();
    trigger.type = map::TriggerType::Lift;
    trigger.target_panel = 0;
    trigger.position = {210, 90};
    trigger.size = {16, 16};
    trigger.activate = map::ActivateType::PlayerPress;

    sim::TriggerSystem triggers;
    triggers.reset(doc);

    EventBus bus;
    int switch_events = 0;
    bus.subscribe<events::WorldSwitchActivated>(
        [&](const events::WorldSwitchActivated&) { ++switch_events; });

    sim::PlayerState player;
    player.snap_to(208.0f, 88.0f);
    triggers.update(player, true, &bus);

    CHECK(switch_events == 1);
    CHECK((triggers.panels()[0].type & map::PANEL_LIFTDOWN) != 0);
}

TEST_CASE("Monster alert fires once when player is spotted", "[sim][audio]") {
    map::MapDocument doc;
    doc.panels.push_back(map::MapPanel{});
    auto& floor = doc.panels.back();
    floor.type = map::PANEL_WALL;
    floor.position = {0, 200};
    floor.size = {512, 16};

    sim::MapCollision collision;
    collision.build_from_panels(doc.panels);

    sim::PlayerState player;
    player.snap_to(200.0f, 140.0f);

    std::vector<sim::ShootableTarget> targets;
    sim::ShootableTarget monster;
    monster.id = 7;
    monster.monster_type = map::MonsterType::Zomby;
    monster.x = 120.0f;
    monster.y = 140.0f;
    monster.prev_x = monster.x;
    monster.prev_y = monster.y;
    monster.width = 34.0f;
    monster.height = 52.0f;
    monster.max_health = 15;
    monster.health = 15;
    targets.push_back(monster);

    EventBus bus;
    int alert_events = 0;
    bus.subscribe<events::MonsterAlerted>([&](const events::MonsterAlerted& event) {
        ++alert_events;
        CHECK(event.entity_id == 7);
        CHECK(event.monster_type == static_cast<std::uint8_t>(map::MonsterType::Zomby));
    });

    sim::MonsterSystem monsters;
    monsters.tick(collision, player, targets, &bus);

    CHECK(alert_events == 1);
    CHECK(targets[0].is_awake);

    monsters.tick(collision, player, targets, &bus);
    CHECK(alert_events == 1);
}
