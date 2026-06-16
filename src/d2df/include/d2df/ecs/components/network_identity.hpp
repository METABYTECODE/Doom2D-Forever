#pragma once

#include <cstdint>

namespace d2df {

struct NetworkIdentity {
    std::uint32_t net_id = 0;
    std::uint8_t owner_peer = 0;
    bool dirty = false;
};

} // namespace d2df
