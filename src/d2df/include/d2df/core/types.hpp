#pragma once

#include <cstdint>

namespace d2df {

using EntityId = std::uint32_t;

constexpr EntityId kInvalidEntity = 0;
constexpr EntityId kPlayerEntityId = 1;

} // namespace d2df
