#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>

#include <d2df/map/map_json_loader.hpp>

#ifndef D2DF_SOURCE_DIR
#define D2DF_SOURCE_DIR "."
#endif

namespace {

std::filesystem::path source_path(const char* rel) {
    return std::filesystem::path(D2DF_SOURCE_DIR) / rel;
}

} // namespace

TEST_CASE("load_map_json_v1 reads 64dm map01", "[map][integration]") {
    const auto map_path = source_path("assets/content/maps/64dm/map01.json");
    if (!std::filesystem::exists(map_path)) {
        SKIP("sample JSON map not available");
    }

    const auto document = d2df::map::load_map_json_v1(map_path);
    REQUIRE(document.version == 1);
    REQUIRE(document.id == "map.64dm.map01");
    REQUIRE(document.size.width == 1600);
    REQUIRE(document.size.height == 1600);
    REQUIRE_FALSE(document.textures.empty());
    REQUIRE(document.textures.size() > document.panels.front().texture);
    REQUIRE(document.music == "music.e1m6");
}

TEST_CASE("load_map_json_v1 reads map items", "[map][integration]") {
    const auto map_path = source_path("assets/content/maps/doom2d/map01.json");
    if (!std::filesystem::exists(map_path)) {
        SKIP("sample JSON map not available");
    }

    const auto document = d2df::map::load_map_json_v1(map_path);

    REQUIRE(document.items.size() > 10);
    REQUIRE(std::any_of(document.items.begin(), document.items.end(),
                        [](const d2df::map::MapItem& item) {
                            return item.type == d2df::map::ItemType::WeaponShotgun1;
                        }));
}
