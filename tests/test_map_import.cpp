#include <catch2/catch_test_macros.hpp>

#include <array>
#include <filesystem>
#include <fstream>

#include <d2df/resources/asset_database.hpp>
#include <d2df/resources/legacy_ref.hpp>
#include <d2df/tools/legacy_map.hpp>
#include <d2df/tools/map_json.hpp>

#ifndef D2DF_SOURCE_DIR
#define D2DF_SOURCE_DIR "."
#endif

namespace {

std::filesystem::path source_path(const char* rel) {
    return std::filesystem::path(D2DF_SOURCE_DIR) / rel;
}

} // namespace

TEST_CASE("normalize_legacy_ref lowercases archive and normalizes slashes", "[resources]") {
    REQUIRE(d2df::resources::normalize_legacy_ref(R"(Standart.wad:D2DMUS\ПРОСТОТА)") ==
            "standart.wad:D2DMUS/ПРОСТОТА");
}

TEST_CASE("qualify_map_legacy_ref resolves map-relative refs", "[resources]") {
    REQUIRE(d2df::resources::qualify_map_legacy_ref(":MUS_DOOM/E1M6", "64DM.wad:MAP01") ==
            "64DM.wad:MUS_DOOM/E1M6");
    REQUIRE(d2df::resources::qualify_map_legacy_ref("standart.WAD:D2DMUS/MENU", "64DM.wad:MAP01") ==
            "standart.WAD:D2DMUS/MENU");
}

TEST_CASE("AssetDatabase loads content manifest and resolves aliases", "[resources][integration]") {
    const auto content = source_path("assets/content");
    if (!std::filesystem::exists(content / "manifest.json")) {
        SKIP("assets/content/manifest.json not available");
    }

    d2df::resources::AssetDatabase db;
    db.load(content);

    const auto* asset = db.get("music.prostota");
    if (asset == nullptr) {
        SKIP("music.prostota not in content catalog");
    }
    REQUIRE(asset->path == "audio/music/prostota.ogg");

    const auto resolved = db.resolve_legacy("standart.wad:D2DMUS/ПРОСТОТА");
    REQUIRE(resolved.has_value());
    REQUIRE(*resolved == "music.prostota");

    const auto bytes = db.read_bytes("music.prostota");
    REQUIRE(bytes.size() > 4);
    REQUIRE(bytes[0] == 'O');
    REQUIRE(bytes[1] == 'g');
    REQUIRE(bytes[2] == 'g');
    REQUIRE(bytes[3] == 'S');
}

TEST_CASE("parse_legacy_mapbin reads 64dm map01 header", "[tools][integration]") {
    const auto map_path = source_path("assets/content/maps/legacy/64dm/map01.mapbin");
    if (!std::filesystem::exists(map_path)) {
        SKIP("sample mapbin not available");
    }

    std::ifstream in(map_path, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)),
                                    std::istreambuf_iterator<char>());
    const auto document = d2df::tools::parse_legacy_mapbin(bytes);

    REQUIRE(document.header.size.width > 0);
    REQUIRE(document.header.size.height > 0);
    REQUIRE_FALSE(document.panels.empty());
}

TEST_CASE("write_map_json_v1 rewrites music and sky refs", "[tools][integration]") {
    const auto content = source_path("assets/content");
    const auto map_path = source_path("assets/content/maps/legacy/64dm/map01.mapbin");
    if (!std::filesystem::exists(content / "manifest.json") ||
        !std::filesystem::exists(map_path)) {
        SKIP("content or sample map not available");
    }

    d2df::resources::ContentCatalog catalog;
    catalog.load(content);

    std::ifstream in(map_path, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)),
                                    std::istreambuf_iterator<char>());
    const auto document = d2df::tools::parse_legacy_mapbin(bytes);

    d2df::tools::MapJsonOptions opts;
    opts.asset_id = "map.64dm.map01";
    opts.legacy_ref = "64DM.wad:MAP01";
    opts.source_path = "maps/legacy/64dm/map01.mapbin";

    std::vector<d2df::tools::MapRefIssue> issues;
    const auto json = d2df::tools::write_map_json_v1(document, catalog, opts, issues);
    REQUIRE(json.find("\"version\": 1") != std::string::npos);
    REQUIRE(json.find("map.64dm.map01") != std::string::npos);
    REQUIRE(json.find("music.e1m6") != std::string::npos);
}
