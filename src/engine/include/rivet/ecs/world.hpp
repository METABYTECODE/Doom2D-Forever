#pragma once

#include <entt/entt.hpp>

namespace rivet::ecs {

using Entity = entt::entity;
inline constexpr Entity kNullEntity = entt::null;

/// Thin wrapper around EnTT registry owned by a scene.
class World {
public:
    [[nodiscard]] entt::registry& registry() { return registry_; }
    [[nodiscard]] const entt::registry& registry() const { return registry_; }

    [[nodiscard]] Entity create();
    void destroy(Entity entity);
    void clear();

private:
    entt::registry registry_;
};

} // namespace rivet::ecs
