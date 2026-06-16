#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>

#include <d2df/tools/json_util.hpp>
#include <d2df/tools/organize_pipeline.hpp>
#include <d2df/tools/staging_manifest.hpp>

#ifndef D2DF_SOURCE_DIR
#define D2DF_SOURCE_DIR "."
#endif

namespace {

std::filesystem::path source_path(const char* rel) {
    return std::filesystem::path(D2DF_SOURCE_DIR) / rel;
}

} // namespace

TEST_CASE("json_extract_string decodes escaped unicode", "[tools]") {
    const std::string line =
        R"({"legacy_ref":"standart.WAD:D2DMUS/TEST","original_name":"ПРОСТОТА"})";
    const auto original = d2df::tools::json_extract_string(line, "original_name");
    REQUIRE(original.has_value());
    REQUIRE(*original == "ПРОСТОТА");
}

TEST_CASE("dedupe prefers data/ over wads/", "[tools]") {
    d2df::tools::StagingRecord from_data;
    from_data.legacy_ref = "standart.WAD:D2DMUS/MENU";
    from_data.disk_path = "data/standart.WAD/D2DMUS/menu.ogg";
    from_data.size_bytes = 100;

    d2df::tools::StagingRecord from_wads;
    from_wads.legacy_ref = "standart.WAD:D2DMUS/MENU";
    from_wads.disk_path = "wads/standart.WAD/D2DMUS/menu.ogg";
    from_wads.size_bytes = 100;

    const auto unique = d2df::tools::dedupe_staging_records({from_wads, from_data});
    REQUIRE(unique.size() == 1);
    REQUIRE(unique.front().disk_path.rfind("data/", 0) == 0);
}

TEST_CASE("organize produces content manifest from staging", "[tools][integration]") {
    const auto staging = source_path("assets/staging");
    const auto manifest = staging / "manifest.json";
    if (!std::filesystem::exists(manifest)) {
        SKIP("assets/staging/manifest.json not available");
    }

    const auto output = std::filesystem::temp_directory_path() / "d2df_organize_test";
    std::filesystem::remove_all(output);

    d2df::tools::OrganizeOptions opts;
    opts.staging_root = staging;
    opts.output_root = output;
    opts.overrides_path = source_path("assets/organize_overrides.json");

    std::vector<d2df::tools::ContentAsset> assets;
    const auto stats = d2df::tools::run_organize(opts, assets);

    REQUIRE(stats.errors == 0);
    REQUIRE(stats.unique_entries > 1000);
    REQUIRE(stats.organized == stats.unique_entries);
    REQUIRE(std::filesystem::exists(output / "manifest.json"));
    REQUIRE(std::filesystem::exists(output / "aliases.json"));

    const auto music_it = std::find_if(
        assets.begin(), assets.end(), [](const d2df::tools::ContentAsset& asset) {
            return asset.slug == "prostota";
        });
    REQUIRE(music_it != assets.end());
    REQUIRE(music_it->id == "music.prostota");
    REQUIRE(music_it->path == "audio/music/prostota.ogg");

    std::filesystem::remove_all(output);
}
