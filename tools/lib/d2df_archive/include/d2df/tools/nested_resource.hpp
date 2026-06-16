#pragma once

#include <d2df/tools/format_sniff.hpp>

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace d2df::tools {

struct NestedAnimInfo {
    std::string resource;
    std::uint32_t frame_width = 0;
    std::uint32_t frame_height = 0;
    std::uint32_t frame_count = 0;
    std::uint32_t wait_count = 0;
    bool back_animation = false;
};

struct UnwrapResult {
    std::vector<std::uint8_t> bytes;
    bool was_nested = false;
    std::optional<NestedAnimInfo> anim;
};

[[nodiscard]] bool is_nested_archive(std::span<const std::uint8_t> data);

[[nodiscard]] UnwrapResult unwrap_nested_resource(std::span<const std::uint8_t> data);

[[nodiscard]] FormatInfo sniff_format_with_name(std::span<const std::uint8_t> data,
                                                std::string_view original_name);

} // namespace d2df::tools
