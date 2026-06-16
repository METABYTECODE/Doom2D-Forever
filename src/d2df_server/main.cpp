#include <spdlog/spdlog.h>

#include <d2df/core/event_bus.hpp>
#include <d2df/core/game_events.hpp>
#include <d2df/core/log.hpp>
#include <d2df/core/service_registry.hpp>
#include <d2df/net/local_simulation_authority.hpp>
#include <d2df/net/null_network_transport.hpp>
#include <d2df/net/simulation_authority.hpp>
#include <d2df/net/network_transport.hpp>

int main() {
    d2df::init_logging();
    spdlog::info("d2df_server — headless stub (Phase 0)");

    d2df::ServiceRegistry services;
    services.register_service<d2df::ISimulationAuthority>(
        std::make_shared<d2df::LocalSimulationAuthority>());
    services.register_service<d2df::INetworkTransport>(
        std::make_shared<d2df::NullNetworkTransport>());

    d2df::EventBus bus;
    bool ticked = false;
    bus.subscribe<d2df::events::AppStarted>([&](const d2df::events::AppStarted&) {
        ticked = true;
        spdlog::debug("Server tick");
    });

    bus.publish(d2df::events::AppStarted{});

    if (!ticked) {
        spdlog::error("EventBus did not dispatch");
        return 1;
    }

    spdlog::info("Server stub OK (authority={}, connected={})",
                 services.get<d2df::ISimulationAuthority>().is_authoritative(),
                 services.get<d2df::INetworkTransport>().is_connected());

    return 0;
}
