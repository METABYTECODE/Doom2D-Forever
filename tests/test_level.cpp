#include <catch2/catch_test_macros.hpp>

#include <filesystem>

#include <rivet/ecs/components/transform.hpp>
#include <rivet/game/level/level_loader.hpp>
#include <rivet/game/level/level_saver.hpp>
#include <rivet/game/level/level_spawner.hpp>
#include <rivet/game/tileset/tileset_catalog.hpp>
#include <rivet/game/resources/resource_pack.hpp>

#ifndef D2DF_SOURCE_DIR
#define D2DF_SOURCE_DIR "."
#endif

namespace {

std::filesystem::path source_path(const char* rel) {
    return std::filesystem::path(D2DF_SOURCE_DIR) / rel;
}

} // namespace

TEST_CASE("ResourcePack loads dev pack from assets", "[game][resources]") {
    const auto assets = source_path("assets");
    if (!std::filesystem::exists(assets / "resourcepacks" / "dev" / "pack.json")) {
        SKIP("dev resource pack not available");
    }

    const auto pack = rivet::game::resources::ResourcePack::load(assets, "dev");
    REQUIRE(pack.has_value());
    CHECK(pack->manifest().id == "dev");
    CHECK(std::filesystem::exists(pack->tilesets_dir()));
}

TEST_CASE("tile_cell_span matches editor footprint for 36px tiles", "[game][tileset]") {
    CHECK(rivet::game::tileset::tile_cell_span(36, 8) == 5);
    CHECK(rivet::game::tileset::tile_cell_span(16, 8) == 2);
    CHECK(rivet::game::tileset::tile_cell_span(8, 8) == 1);
}

TEST_CASE("Level loader reads rivet-level JSON", "[game][level]") {
    const auto level_path = source_path("assets/levels/test.level.json");
    if (!std::filesystem::exists(level_path)) {
        SKIP("sample level file not available");
    }

    const auto level = rivet::game::level::load_level(level_path);

    CHECK(level.name == "sample");
    CHECK(level.grid_size == 8);
    CHECK(level.width > 0);
    CHECK(level.height > 0);
    REQUIRE(level.collision.size() == static_cast<std::size_t>(level.height));
    bool has_solid = false;
    for (const auto& row : level.collision) {
        for (const int cell : row) {
            if (cell == 1) {
                has_solid = true;
                break;
            }
        }
        if (has_solid) {
            break;
        }
    }
    CHECK(has_solid);
    CHECK(level.collision[2][2] == 0);
    REQUIRE(level.objects.size() >= 1);
    CHECK(level.objects.front().type == "player");
}

TEST_CASE("Level spawner builds ECS from collision grid", "[game][level]") {
    const auto level_path = source_path("assets/levels/test.level.json");
    if (!std::filesystem::exists(level_path)) {
        SKIP("sample level file not available");
    }

    const auto level = rivet::game::level::load_level(level_path);
    rivet::ecs::World world;

    const auto result = rivet::game::level::spawn_level(world, level);
    CHECK(result.player != rivet::ecs::kNullEntity);
    CHECK(result.world_width == 640.0f);
    CHECK(result.world_height == 480.0f);
    const auto view = world.registry().view<rivet::ecs::components::Transform>();
    CHECK(view.size() > 0);
}

TEST_CASE("Level saver round-trips rivet-level v2 JSON", "[game][level]") {
    rivet::game::level::LevelData original;
    original.name = "roundtrip";
    original.grid_size = 8;
    original.width = 10;
    original.height = 8;
    original.collision = std::vector<std::vector<int>>(8, std::vector<int>(10, 0));
    original.collision[0][0] = 1;
    original.collision[1][1] = 1;
    original.tiles.push_back({
        .tileset = "terrain",
        .id = 2,
        .x = 3,
        .y = 4,
        .frames =
            {
                {.tileset = "terrain", .id = 2},
                {.tileset = "terrain", .id = 3},
                {.tileset = "terrain", .id = 4},
            },
        .frame_ms = 150,
    });
    original.objects.push_back(
        {.id = "player", .type = "player", .x = 32.0f, .y = 48.0f, .width = 40.0f, .height = 40.0f});

    const auto temp_path = std::filesystem::temp_directory_path() / "rivet_level_roundtrip.json";
    rivet::game::level::save_level(temp_path, original);
    const auto reloaded = rivet::game::level::load_level(temp_path);

    CHECK(reloaded.name == original.name);
    CHECK(reloaded.grid_size == original.grid_size);
    CHECK(reloaded.width == original.width);
    CHECK(reloaded.height == original.height);
    CHECK(reloaded.collision == original.collision);
    REQUIRE(reloaded.tiles.size() == 1);
    CHECK(reloaded.tiles.front().tileset == "terrain");
    CHECK(reloaded.tiles.front().id == 2);
    REQUIRE(reloaded.tiles.front().frames.size() == 3);
    CHECK(reloaded.tiles.front().frames[1].id == 3);
    CHECK(reloaded.tiles.front().frame_ms == 150);
    REQUIRE(reloaded.objects.size() == 1);
    CHECK(reloaded.objects.front().type == "player");

    std::filesystem::remove(temp_path);
}
