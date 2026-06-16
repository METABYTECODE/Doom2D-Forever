#include <d2df/map/key_types.hpp>

namespace d2df::map {

std::uint8_t key_mask_from_name(std::string_view name) {
    if (name == "KEY_RED") {
        return KeyRed;
    }
    if (name == "KEY_GREEN") {
        return KeyGreen;
    }
    if (name == "KEY_BLUE") {
        return KeyBlue;
    }
    if (name == "KEY_REDTEAM") {
        return KeyRedTeam;
    }
    if (name == "KEY_BLUETEAM") {
        return KeyBlueTeam;
    }
    return KeyNone;
}

std::uint8_t key_mask_from_json_array(std::string_view array_snippet) {
    std::uint8_t mask = KeyNone;
    std::size_t pos = 0;
    while ((pos = array_snippet.find('"', pos)) != std::string_view::npos) {
        const auto name_start = pos + 1;
        const auto name_end = array_snippet.find('"', name_start);
        if (name_end == std::string_view::npos) {
            break;
        }
        mask |= key_mask_from_name(array_snippet.substr(name_start, name_end - name_start));
        pos = name_end + 1;
    }
    return mask;
}

} // namespace d2df::map
