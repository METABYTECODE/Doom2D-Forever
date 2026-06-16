#pragma once

#include <span>
#include <vector>

namespace d2df::tools {

[[nodiscard]] std::vector<std::uint8_t> zlib_inflate_raw(std::span<const std::uint8_t> compressed,
                                                        std::size_t hint_size = 0);

} // namespace d2df::tools
