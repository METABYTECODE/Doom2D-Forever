#include <d2df/tools/nested_resource.hpp>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>

#include <d2df/tools/archive.hpp>
#include <d2df/tools/format_sniff.hpp>

namespace d2df::tools {
namespace {

std::string lowercase_ascii(std::string_view text) {
    std::string out(text);
    for (char& c : out) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return out;
}

std::string entry_full_path(const ArchiveEntry& entry) {
    return entry.path + entry.name;
}

const ArchiveEntry* find_entry(const ArchiveFile& archive, std::string_view full_path) {
    const auto target = lowercase_ascii(full_path);
    for (const auto& entry : archive.entries) {
        if (entry.is_directory) {
            continue;
        }
        if (lowercase_ascii(entry_full_path(entry)) == target) {
            return &entry;
        }
    }
    return nullptr;
}

std::optional<NestedAnimInfo> parse_anim_ini(std::span<const std::uint8_t> data) {
    const std::string text(reinterpret_cast<const char*>(data.data()), data.size());
    NestedAnimInfo info;
    bool found_resource = false;

    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const auto eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }

        const auto key = lowercase_ascii(line.substr(0, eq));
        const auto value = line.substr(eq + 1);
        if (key == "resource") {
            info.resource = value;
            found_resource = !info.resource.empty();
        } else if (key == "framewidth") {
            info.frame_width = static_cast<std::uint32_t>(std::stoul(value));
        } else if (key == "frameheight") {
            info.frame_height = static_cast<std::uint32_t>(std::stoul(value));
        } else if (key == "framecount") {
            info.frame_count = static_cast<std::uint32_t>(std::stoul(value));
        } else if (key == "waitcount") {
            info.wait_count = static_cast<std::uint32_t>(std::stoul(value));
        } else if (key == "backanimation") {
            info.back_animation = value == "1" || lowercase_ascii(value) == "true";
        }
    }

    if (!found_resource) {
        return std::nullopt;
    }
    return info;
}

bool is_image_kind(AssetKind kind) {
    return kind == AssetKind::Image;
}

} // namespace

bool is_nested_archive(std::span<const std::uint8_t> data) {
    return detect_archive_type(data) != ArchiveType::Unknown;
}

UnwrapResult unwrap_nested_resource(std::span<const std::uint8_t> data) {
    UnwrapResult result;
    result.bytes.assign(data.begin(), data.end());

    if (!is_nested_archive(data)) {
        return result;
    }

    std::vector<std::uint8_t> copy(data.begin(), data.end());
    ArchiveFile archive;
    try {
        archive = open_archive_bytes(std::move(copy), "nested");
    } catch (...) {
        return result;
    }

    if (const auto* anim_entry = find_entry(archive, "TEXT/ANIM")) {
        try {
            const auto anim_bytes = extract_entry(archive, *anim_entry);
            const auto anim_info = parse_anim_ini(anim_bytes);
            if (anim_info) {
                const auto texture_path = "TEXTURES/" + anim_info->resource;
                if (const auto* texture_entry = find_entry(archive, texture_path)) {
                    result.bytes = extract_entry(archive, *texture_entry);
                    result.was_nested = true;
                    result.anim = anim_info;
                    return result;
                }
            }
        } catch (...) {
            // fall through to generic nested extraction
        }
    }

    for (const auto& entry : archive.entries) {
        if (entry.is_directory) {
            continue;
        }
        if (entry.path.rfind("TEXTURES/", 0) != 0 && entry.path != "TEXTURES/") {
            continue;
        }
        try {
            const auto candidate = extract_entry(archive, entry);
            if (is_image_kind(sniff_format(candidate).kind)) {
                result.bytes = std::move(candidate);
                result.was_nested = true;
                return result;
            }
        } catch (...) {
            continue;
        }
    }

    for (const auto& entry : archive.entries) {
        if (entry.is_directory) {
            continue;
        }
        try {
            const auto candidate = extract_entry(archive, entry);
            const auto format = sniff_format(candidate);
            if (is_image_kind(format.kind) || format.kind == AssetKind::Audio) {
                result.bytes = std::move(candidate);
                result.was_nested = true;
                return result;
            }
        } catch (...) {
            continue;
        }
    }

    return result;
}

FormatInfo sniff_format_with_name(std::span<const std::uint8_t> data,
                                  std::string_view original_name) {
    auto info = sniff_format(data);
    if (info.kind != AssetKind::Binary) {
        return info;
    }

    const auto dot = original_name.rfind('.');
    if (dot == std::string_view::npos) {
        return info;
    }

    const auto ext = lowercase_ascii(original_name.substr(dot + 1));
    if (ext == "jpg" || ext == "jpeg" || ext == "jpe") {
        info.kind = AssetKind::Image;
        info.extension = "jpg";
        info.mime = "image/jpeg";
    } else if (ext == "png") {
        info.kind = AssetKind::Image;
        info.extension = "png";
        info.mime = "image/png";
    } else if (ext == "tga") {
        info.kind = AssetKind::Image;
        info.extension = "tga";
        info.mime = "image/tga";
    } else if (ext == "bmp") {
        info.kind = AssetKind::Image;
        info.extension = "bmp";
        info.mime = "image/bmp";
    } else if (ext == "gif") {
        info.kind = AssetKind::Image;
        info.extension = "gif";
        info.mime = "image/gif";
    } else if (ext == "mid" || ext == "midi") {
        info.kind = AssetKind::Audio;
        info.extension = "mid";
        info.mime = "audio/midi";
    } else if (ext == "wav") {
        info.kind = AssetKind::Audio;
        info.extension = "wav";
        info.mime = "audio/wav";
    } else if (ext == "ogg") {
        info.kind = AssetKind::Audio;
        info.extension = "ogg";
        info.mime = "audio/ogg";
    } else if (ext == "mp3") {
        info.kind = AssetKind::Audio;
        info.extension = "mp3";
        info.mime = "audio/mpeg";
    }

    return info;
}

} // namespace d2df::tools
