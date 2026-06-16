#include <catch2/catch_test_macros.hpp>

#include <array>
#include <filesystem>
#include <fstream>

#include <d2df/tools/archive.hpp>
#include <d2df/tools/format_sniff.hpp>
#include <d2df/tools/nested_resource.hpp>
#include <d2df/tools/text_encoding.hpp>

#ifndef D2DF_SOURCE_DIR
#define D2DF_SOURCE_DIR "."
#endif

namespace {

std::filesystem::path source_path(const char* rel) {
    return std::filesystem::path(D2DF_SOURCE_DIR) / rel;
}

} // namespace

TEST_CASE("CP1251 decodes Russian music title", "[tools]") {
    // "ПРОСТОТА" uppercase in CP1251
    const std::string cp1251{'\xCF', '\xD0', '\xCE', '\xD1', '\xD2', '\xCE', '\xD2', '\xC0'};
    const auto utf8 = d2df::tools::cp1251_to_utf8(cp1251);
    const std::string expected = "\xD0\x9F\xD0\xA0\xD0\x9E\xD0\xA1\xD0\xA2\xD0\x9E\xD0\xA2\xD0\x90";
    REQUIRE(utf8 == expected);
}

TEST_CASE("DFWAD opens game.WAD when present", "[tools][integration]") {
    const auto path = source_path("assets/data/game.WAD");
    if (!std::filesystem::exists(path)) {
        SKIP("assets/data/game.WAD not available");
    }

    const auto archive = d2df::tools::open_archive_file(path.string());
    REQUIRE(archive.type == d2df::tools::ArchiveType::Dfwad);
    REQUIRE(archive.entries.size() > 100);
}

TEST_CASE("DFWAD extract first texture entry", "[tools][integration]") {
    const auto path = source_path("assets/data/game.WAD");
    if (!std::filesystem::exists(path)) {
        SKIP("assets/data/game.WAD not available");
    }

    const auto archive = d2df::tools::open_archive_file(path.string());
    const d2df::tools::ArchiveEntry* entry = nullptr;
    for (const auto& e : archive.entries) {
        if (!e.is_directory && e.name == "STDTXT" && e.packed_size > 0) {
            entry = &e;
            break;
        }
    }
    REQUIRE(entry != nullptr);

    const auto bytes = d2df::tools::extract_entry(archive, *entry);
    REQUIRE(bytes.size() > 8);
}

TEST_CASE("nested anim texture unwraps to TGA image bytes", "[tools]") {
    const auto path = source_path("assets/staging/data/standart.WAD/D2DTEXTURES/col5a0.bin");
    if (!std::filesystem::exists(path)) {
        SKIP("col5a0.bin not available — run d2df-extract first");
    }

    std::ifstream in(path, std::ios::binary);
    std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    REQUIRE(d2df::tools::is_nested_archive(data));
    const auto unwrapped = d2df::tools::unwrap_nested_resource(data);
    REQUIRE(unwrapped.was_nested);
    REQUIRE(unwrapped.anim.has_value());
    REQUIRE(unwrapped.anim->resource == "COL5A");
    REQUIRE(unwrapped.anim->frame_count == 2);

    const auto format = d2df::tools::sniff_format(unwrapped.bytes);
    REQUIRE(format.kind == d2df::tools::AssetKind::Image);
    REQUIRE(format.extension == "tga");
}

TEST_CASE("JPEG and MIDI formats are detected", "[tools]") {
    const std::array<std::uint8_t, 4> jpeg_magic{0xFF, 0xD8, 0xFF, 0xE0};
    const auto jpeg = d2df::tools::sniff_format(jpeg_magic);
    REQUIRE(jpeg.kind == d2df::tools::AssetKind::Image);
    REQUIRE(jpeg.extension == "jpg");

    const std::array<std::uint8_t, 4> midi_magic{'M', 'T', 'h', 'd'};
    const auto midi = d2df::tools::sniff_format(midi_magic);
    REQUIRE(midi.kind == d2df::tools::AssetKind::Audio);
    REQUIRE(midi.extension == "mid");
}

TEST_CASE("XM Extended Module is detected", "[tools]") {
    const std::string xm_header = "Extended Module: test";
    const auto bytes = std::vector<std::uint8_t>(xm_header.begin(), xm_header.end());
    const auto format = d2df::tools::sniff_format(bytes);
    REQUIRE(format.kind == d2df::tools::AssetKind::Audio);
    REQUIRE(format.extension == "xm");
}

TEST_CASE("MIDI is not sent to ffmpeg for conversion", "[tools]") {
    REQUIRE_FALSE(d2df::tools::ffmpeg_can_convert_audio("mid"));
    REQUIRE_FALSE(d2df::tools::ffmpeg_can_convert_audio("midi"));
    REQUIRE(d2df::tools::ffmpeg_can_convert_audio("xm"));
    REQUIRE(d2df::tools::ffmpeg_can_convert_audio("wav"));
}
