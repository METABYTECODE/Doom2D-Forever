#include <catch2/catch_test_macros.hpp>

#include <filesystem>

#include <rivet/ecs/components/transform.hpp>
#include <rivet/game/level/level_loader.hpp>
#include <rivet/game/level/level_spawner.hpp>

#ifndef D2DF_SOURCE_DIR
#define D2DF_SOURCE_DIR "."
#endif

namespace {

std::filesystem::path source_path(const char* rel) {
    return std::filesystem::path(D2DF_SOURCE_DIR) / rel;
}

} // namespace

TEST_CASE("Level loader reads rivet-level JSON", "[game][level]") {
    const auto level_path = source_path("assets/levels/test.level.json");
    if (!std::filesystem::exists(level_path)) {
        SKIP("sample level file not available");
    }

    const auto level = rivet::game::level::load_level(level_path);

    CHECK(level.name == "test");
    CHECK(level.tile_size == 32);
    CHECK(level.width == 20);
    CHECK(level.height == 15);
    REQUIRE(level.tiles.size() == 15);
    CHECK(level.tiles.front().front() == 1);
    CHECK(level.tiles[1][1] == 0);
    REQUIRE(level.objects.size() == 2);
    CHECK(level.objects.front().type == "player");
    CHECK(level.objects.front().x == 96.0f);
}

TEST_CASE("Level spawner builds ECS from data", "[game][level]") {
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
