#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>

#include <rivet/ecs/components/collider.hpp>
#include <rivet/ecs/components/transform.hpp>
#include <rivet/game/level/level_loader.hpp>
#include <rivet/game/level/level_saver.hpp>
#include <rivet/game/level/level_spawner.hpp>
#include <rivet/game/tileset/tileset_catalog.hpp>
#include <rivet/game/resources/resource_pack.hpp>
#include <rivet/game/model/model_animator.hpp>
#include <rivet/game/model/model_types.hpp>

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
    REQUIRE(!level.tile_layers.empty());
    CHECK(!level.tile_layers.front().tiles.empty());
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

    const auto assets = source_path("assets");
    if (std::filesystem::exists(assets / "resourcepacks" / "dev" / "pack.json")) {
        const auto pack = rivet::game::resources::ResourcePack::load(assets, "dev");
        REQUIRE(pack.has_value());
        rivet::ecs::World model_world;
        const auto model_result =
            rivet::game::level::spawn_level(model_world, level, {.pack = &*pack});
        const auto& collider =
            model_world.registry().get<rivet::ecs::components::Collider>(model_result.player);
        CHECK(collider.width == 34.0f);
        CHECK(collider.height == 52.0f);
        CHECK(collider.offset_x == -17.0f);
        CHECK(collider.offset_y == -52.0f);
    }

    const auto view = world.registry().view<rivet::ecs::components::Transform>();
    CHECK(view.size() > 0);
}

TEST_CASE("Level saver round-trips rivet-level v4 JSON", "[game][level]") {
    rivet::game::level::LevelData original;
    original.name = "roundtrip";
    original.grid_size = 16;
    original.width = 80;
    original.height = 64;
    original.collision = std::vector<std::vector<int>>(4, std::vector<int>(5, 0));
    original.collision[0][0] = 1;
    original.collision[1][1] = 1;

    rivet::game::level::TileLayer main_layer;
    main_layer.id = "main";
    main_layer.z = 0;
    main_layer.tiles.push_back({
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
    original.tile_layers.push_back(main_layer);

    rivet::game::level::TileLayer fore_layer;
    fore_layer.id = "foreground";
    fore_layer.z = 100;
    original.tile_layers.push_back(fore_layer);
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
    REQUIRE(reloaded.tile_layers.size() == 2);
    REQUIRE(reloaded.tile_layers.front().tiles.size() == 1);
    CHECK(reloaded.tile_layers.front().tiles.front().tileset == "terrain");
    CHECK(reloaded.tile_layers.front().tiles.front().id == 2);
    CHECK(reloaded.tile_layers.front().tiles.front().x == 48);
    CHECK(reloaded.tile_layers.front().tiles.front().y == 32);
    REQUIRE(reloaded.tile_layers.front().tiles.front().frames.size() == 3);
    CHECK(reloaded.tile_layers.front().tiles.front().frames[1].id == 3);
    CHECK(reloaded.tile_layers.front().tiles.front().frame_ms == 150);
    CHECK(reloaded.tile_layers[1].id == "foreground");
    CHECK(reloaded.tile_layers[1].z == 100);
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

TEST_CASE("Player locomotion selects built-in animation ids", "[game][model]") {
    using rivet::game::model::select_player_locomotion_animation;

    CHECK(select_player_locomotion_animation(true, 0.0f, 0.0f) == "IDLE");
    CHECK(select_player_locomotion_animation(true, 120.0f, 0.0f) == "WALK");
    CHECK(select_player_locomotion_animation(false, 0.0f, -200.0f) == "JUMP");
    CHECK(select_player_locomotion_animation(false, 0.0f, 200.0f) == "FALL");
}

TEST_CASE("ModelAnimator advances looping clip frames", "[game][model]") {
    rivet::game::model::ModelDef model;
    model.id = "test";

    rivet::game::model::ModelAnimation clip;
    clip.frame_width = 32;
    clip.frame_height = 32;
    clip.columns = 2;
    clip.frame_ids = {0, 1};
    clip.frame_ms = 100;
    clip.loop = true;
    model.animations.emplace("IDLE", clip);

    rivet::game::model::ModelAnimator animator;
    animator.set_model(&model);
    animator.set_animation("IDLE", true);
    animator.update(0.15f);

    CHECK(animator.current_frame().source.x == 32.0f);
    CHECK(animator.current_frame().source.y == 0.0f);
}

TEST_CASE("Model sprite dest rect uses pivot in frame space", "[game][model]") {
    const rivet::game::model::ModelPivot pivot{
        .x = 32.0f,
        .y = 64.0f,
    };
    const auto dest = rivet::game::model::model_sprite_dest_rect(100.0f, 200.0f, pivot, 64, 64);
    CHECK(dest.x == 68.0f);
    CHECK(dest.y == 136.0f);
    CHECK(dest.width == 64.0f);
    CHECK(dest.height == 64.0f);
}

TEST_CASE("ResourcePack resolves player model path", "[game][model]") {
    const auto assets = source_path("assets");
    if (!std::filesystem::exists(assets / "resourcepacks" / "dev" / "pack.json")) {
        SKIP("dev resource pack not available");
    }

    const auto pack = rivet::game::resources::ResourcePack::load(assets, "dev");
    REQUIRE(pack.has_value());

    const auto model_path = pack->resolve_model("player");
    REQUIRE(model_path.has_value());
    CHECK(std::filesystem::exists(*model_path));
}
