#pragma once

namespace d2df {

enum class Authority {
    Local,
    Server,
    Client,
};

class ISimulationAuthority {
public:
    virtual ~ISimulationAuthority() = default;

    [[nodiscard]] virtual Authority mode() const = 0;
    [[nodiscard]] virtual bool is_authoritative() const = 0;
};

} // namespace d2df
