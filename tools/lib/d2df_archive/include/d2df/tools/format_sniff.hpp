#pragma once

#include <cstdint>
#include <span>
#include <string>

namespace d2df::tools {

enum class AssetKind {
    Unknown,
    Image,
    Audio,
    Map,
    FontConfig,
    Binary,
};

struct FormatInfo {
    AssetKind kind = AssetKind::Unknown;
    std::string extension; // suggested, without dot
    std::string mime;
};

[[nodiscard]] FormatInfo sniff_format(std::span<const std::uint8_t> data);

// MIDI cannot be rendered to OGG without an external synthesizer; ffmpeg essentials has no SMF demuxer.
[[nodiscard]] bool ffmpeg_can_convert_audio(std::string_view extension);

} // namespace d2df::tools
