#pragma once

#include <d2df/net/simulation_authority.hpp>

namespace d2df {

/// Single-player / offline: local simulation is always authoritative.
class LocalSimulationAuthority final : public ISimulationAuthority {
public:
    [[nodiscard]] Authority mode() const override { return Authority::Local; }

    [[nodiscard]] bool is_authoritative() const override { return true; }
};

} // namespace d2df
