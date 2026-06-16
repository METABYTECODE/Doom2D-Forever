#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace d2df::tools {

[[nodiscard]] std::string json_escape(std::string_view text);

[[nodiscard]] std::optional<std::string> json_extract_string(std::string_view object,
                                                             std::string_view key);

[[nodiscard]] std::optional<std::uint64_t> json_extract_uint64(std::string_view object,
                                                               std::string_view key);

} // namespace d2df::tools
