#include <catch2/catch_test_macros.hpp>

#include <memory>

#include <d2df/core/service_registry.hpp>
#include <d2df/ecs/components/network_identity.hpp>
#include <d2df/net/local_simulation_authority.hpp>
#include <d2df/net/network_transport.hpp>
#include <d2df/net/null_network_transport.hpp>
#include <d2df/net/simulation_authority.hpp>

TEST_CASE("ServiceRegistry resolves registered services", "[core]") {
    d2df::ServiceRegistry registry;
    registry.register_service<d2df::ISimulationAuthority>(
        std::make_shared<d2df::LocalSimulationAuthority>());
    registry.register_service<d2df::INetworkTransport>(
        std::make_shared<d2df::NullNetworkTransport>());

    REQUIRE(registry.get<d2df::ISimulationAuthority>().is_authoritative());
    REQUIRE_FALSE(registry.get<d2df::INetworkTransport>().is_connected());
}

TEST_CASE("NetworkIdentity default state", "[ecs]") {
    d2df::NetworkIdentity id{};
    REQUIRE(id.net_id == 0);
    REQUIRE(id.owner_peer == 0);
    REQUIRE_FALSE(id.dirty);
}
