#include <catch2/catch_test_macros.hpp>

#include <algorithm>
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

[[nodiscard]] int sub_grid_cols(const int width_px, const int grid_size) {
    return std::max(1, (width_px + grid_size - 1) / grid_size);
}

[[nodiscard]] int sub_grid_rows(const int height_px, const int grid_size) {
    return std::max(1, (height_px + grid_size - 1) / grid_size);
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

TEST_CASE("Level loader reads rivet-level v3 JSON", "[game][level]") {
    const auto level_path = source_path("assets/levels/test.level.json");
    if (!std::filesystem::exists(level_path)) {
        SKIP("test level file not available");
    }

    const auto level = rivet::game::level::load_level(level_path);
    const int cols = sub_grid_cols(level.width, level.grid_size);
    const int rows = sub_grid_rows(level.height, level.grid_size);

    CHECK(level.grid_size == 16);
    CHECK(level.width == 1600);
    CHECK(level.height == 1600);
    REQUIRE(level.collision.size() == static_cast<std::size_t>(rows));
    REQUIRE(!level.collision.empty());
    CHECK(level.collision.front().size() == static_cast<std::size_t>(cols));
    REQUIRE(!level.tiles.empty());
    CHECK(level.tiles.front().x == 96);
    CHECK(level.tiles.front().y == 112);
    REQUIRE(level.objects.size() >= 1);
    CHECK(level.objects.front().type == "player");
}

TEST_CASE("Level spawner builds ECS from collision grid", "[game][level]") {
    const auto level_path = source_path("assets/levels/test.level.json");
    if (!std::filesystem::exists(level_path)) {
        SKIP("test level file not available");
    }

    const auto level = rivet::game::level::load_level(level_path);
    rivet::ecs::World world;

    const auto result = rivet::game::level::spawn_level(world, level);
    CHECK(result.player != rivet::ecs::kNullEntity);
    CHECK(result.world_width == 1600.0f);
    CHECK(result.world_height == 1600.0f);
    const auto view = world.registry().view<rivet::ecs::components::Transform>();
    CHECK(view.size() > 0);
}

TEST_CASE("Level saver round-trips rivet-level v3 JSON", "[game][level]") {
    rivet::game::level::LevelData original;
    original.name = "roundtrip";
    original.grid_size = 16;
    original.width = 80;
    original.height = 64;
    original.collision = std::vector<std::vector<int>>(4, std::vector<int>(5, 0));
    original.collision[0][0] = 1;
    original.collision[1][1] = 1;
    original.tiles.push_back({
        .tileset = "terrain",
        .id = 2,
        .x = 48,
        .y = 32,
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
    CHECK(reloaded.tiles.front().x == 48);
    CHECK(reloaded.tiles.front().y == 32);
    REQUIRE(reloaded.tiles.front().frames.size() == 3);
    CHECK(reloaded.tiles.front().frames[1].id == 3);
    CHECK(reloaded.tiles.front().frame_ms == 150);
    REQUIRE(reloaded.objects.size() == 1);
    CHECK(reloaded.objects.front().type == "player");

    std::filesystem::remove(temp_path);
}

TEST_CASE("ResourcePack resolves nested music paths", "[game][resources]") {
    const auto assets = source_path("assets");
    if (!std::filesystem::exists(assets / "resourcepacks" / "dev" / "pack.json")) {
        SKIP("dev resource pack not available");
    }

    const auto pack = rivet::game::resources::ResourcePack::load(assets, "dev");
    REQUIRE(pack.has_value());

    const auto music = pack->resolve_music("minidoom/Music/0055");
    if (!music.has_value()) {
        SKIP("sample music track not available in dev pack");
    }
    CHECK(std::filesystem::exists(*music));
}

TEST_CASE("Tileset anchor maps world pixel to destination rect", "[game][level]") {
    rivet::game::tileset::TilesetDef tileset;
    tileset.tile_width = 16;
    tileset.tile_height = 16;
    tileset.anchor = rivet::game::tileset::TileAnchor::BottomLeft;

    const auto dest = rivet::game::tileset::tile_dest_rect(tileset, 96.0f, 112.0f);
    CHECK(dest.x == 96.0f);
    CHECK(dest.y == 96.0f);
    CHECK(dest.width == 16.0f);
    CHECK(dest.height == 16.0f);
}
