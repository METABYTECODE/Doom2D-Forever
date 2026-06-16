#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace d2df::tools {

[[nodiscard]] std::vector<std::string> bitset_names(const std::pair<std::uint16_t, const char*>* flags,
                                                    std::size_t count, std::uint16_t value);

[[nodiscard]] std::string dir_type_name(std::uint8_t value);
[[nodiscard]] std::string item_type_name(std::uint8_t value);
[[nodiscard]] std::string monster_type_name(std::uint8_t value);
[[nodiscard]] std::string area_type_name(std::uint8_t value);
[[nodiscard]] std::string trigger_type_name(std::uint8_t value);

[[nodiscard]] std::vector<std::string> panel_type_names(std::uint16_t value);
[[nodiscard]] std::vector<std::string> panel_flag_names(std::uint8_t value);
[[nodiscard]] std::vector<std::string> activate_type_names(std::uint8_t value);
[[nodiscard]] std::vector<std::string> key_names(std::uint8_t value);
[[nodiscard]] std::vector<std::string> item_option_names(std::uint8_t value);

} // namespace d2df::tools
