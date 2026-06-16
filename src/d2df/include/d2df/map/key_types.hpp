#pragma once

#include <cstdint>
#include <string_view>

namespace d2df::map {

enum KeyMask : std::uint8_t {
    KeyNone = 0,
    KeyRed = 1,
    KeyGreen = 2,
    KeyBlue = 4,
    KeyRedTeam = 8,
    KeyBlueTeam = 16,
};

[[nodiscard]] std::uint8_t key_mask_from_name(std::string_view name);
[[nodiscard]] std::uint8_t key_mask_from_json_array(std::string_view array_snippet);

} // namespace d2df::map
