#pragma once

namespace d2df {

class INetworkTransport {
public:
    virtual ~INetworkTransport() = default;

    virtual void poll() = 0;
    [[nodiscard]] virtual bool is_connected() const = 0;
};

} // namespace d2df
