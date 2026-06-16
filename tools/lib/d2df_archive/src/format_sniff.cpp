#include <d2df/tools/format_sniff.hpp>

#include <algorithm>
#include <cstring>
#include <string_view>

namespace d2df::tools {

FormatInfo sniff_format(std::span<const std::uint8_t> data) {
    FormatInfo info;

    if (data.size() >= 4) {
        if (data[0] == 'M' && data[1] == 'A' && data[2] == 'P' && data[3] == 0x01) {
            info.kind = AssetKind::Map;
            info.extension = "mapbin";
            info.mime = "application/x-d2df-map";
            return info;
        }
        if (data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F' && data.size() >= 12 &&
            data[8] == 'W' && data[9] == 'A' && data[10] == 'V' && data[11] == 'E') {
            info.kind = AssetKind::Audio;
            info.extension = "wav";
            info.mime = "audio/wav";
            return info;
        }
        if (data[0] == 'O' && data[1] == 'g' && data[2] == 'g' && data[3] == 'S') {
            info.kind = AssetKind::Audio;
            info.extension = "ogg";
            info.mime = "audio/ogg";
            return info;
        }
        if (data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G') {
            info.kind = AssetKind::Image;
            info.extension = "png";
            info.mime = "image/png";
            return info;
        }
        if (data[0] == 'B' && data[1] == 'M') {
            info.kind = AssetKind::Image;
            info.extension = "bmp";
            info.mime = "image/bmp";
            return info;
        }
        if (data[0] == 'G' && data[1] == 'I' && data[2] == 'F') {
            info.kind = AssetKind::Image;
            info.extension = "gif";
            info.mime = "image/gif";
            return info;
        }
        if (data.size() >= 18 && data[0] == 0 && data[1] == 0 && data[2] == 2 &&
            (data[16] == 8 || data[16] == 16 || data[16] == 24 || data[16] == 32)) {
            info.kind = AssetKind::Image;
            info.extension = "tga";
            info.mime = "image/tga";
            return info;
        }
        if (data[0] == 'I' && data[1] == 'D' && data[2] == '3') {
            info.kind = AssetKind::Audio;
            info.extension = "mp3";
            info.mime = "audio/mpeg";
            return info;
        }
        if (data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF) {
            info.kind = AssetKind::Image;
            info.extension = "jpg";
            info.mime = "image/jpeg";
            return info;
        }
        if (data[0] == 'M' && data[1] == 'T' && data[2] == 'h' && data[3] == 'd') {
            info.kind = AssetKind::Audio;
            info.extension = "mid";
            info.mime = "audio/midi";
            return info;
        }
        if (data[0] == 0xFF && (data[1] & 0xE0) == 0xE0 && data[1] != 0xD8) {
            info.kind = AssetKind::Audio;
            info.extension = "mp3";
            info.mime = "audio/mpeg";
            return info;
        }
    }

    if (data.size() >= 2) {
        if (std::memcmp(data.data(), "BM", 2) == 0) {
            info.kind = AssetKind::Image;
            info.extension = "bmp";
            return info;
        }
    }

    // Tracker modules
    if (data.size() >= 17 &&
        std::memcmp(data.data(), "Extended Module:", 16) == 0) {
        info.kind = AssetKind::Audio;
        info.extension = "xm";
        info.mime = "audio/xm";
        return info;
    }
    if (data.size() >= 4) {
        if (std::memcmp(data.data(), "IMPM", 4) == 0) {
            info.kind = AssetKind::Audio;
            info.extension = "it";
            info.mime = "audio/it";
            return info;
        }
        if (data.size() >= 48 && std::memcmp(data.data() + 44, "SCRM", 4) == 0) {
            info.kind = AssetKind::Audio;
            info.extension = "s3m";
            info.mime = "audio/s3m";
            return info;
        }
    }
    if (data.size() >= 1084) {
        const char* mod_magic = reinterpret_cast<const char*>(data.data() + 1080);
        if (std::memcmp(mod_magic, "M.K.", 4) == 0 || std::memcmp(mod_magic, "M!K!", 4) == 0 ||
            std::memcmp(mod_magic, "FLT4", 4) == 0 || std::memcmp(mod_magic, "FLT8", 4) == 0) {
            info.kind = AssetKind::Audio;
            info.extension = "mod";
            info.mime = "audio/mod";
            return info;
        }
    }

    if (data.size() >= 8) {
        const std::string_view head(reinterpret_cast<const char*>(data.data()),
                                    std::min(data.size(), std::size_t{64}));
        if (head.find("[FontMap]") != std::string_view::npos) {
            info.kind = AssetKind::FontConfig;
            info.extension = "cfg";
            info.mime = "text/plain";
            return info;
        }
    }

    info.kind = AssetKind::Binary;
    info.extension = "bin";
    info.mime = "application/octet-stream";
    return info;
}

bool ffmpeg_can_convert_audio(std::string_view extension) {
    if (extension == "ogg") {
        return false;
    }
    // Standard ffmpeg builds (incl. gyan.dev essentials) lack SMF demuxer — MIDI is note data, not PCM.
    if (extension == "mid" || extension == "midi") {
        return false;
    }
    return true;
}

} // namespace d2df::tools
