#pragma once

#include <d2df/net/network_transport.hpp>

namespace d2df {

/// Offline stub — replaced with ENet transport in Phase 9.
class NullNetworkTransport final : public INetworkTransport {
public:
    void poll() override {}

    [[nodiscard]] bool is_connected() const override { return false; }
};

} // namespace d2df
