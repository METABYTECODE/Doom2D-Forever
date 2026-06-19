#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include <entt/entt.hpp>

#include <rivet/ecs/world.hpp>
#include <rivet/physics/aabb.hpp>

namespace rivet::physics {

/// Uniform-grid broadphase for ECS static colliders.
class SpatialGrid {
public:
    void build(const rivet::ecs::World& world, float cell_size = 64.0f);

    void for_each_in_aabb(
        const Aabb& box,
        const std::function<void(entt::entity entity, const Aabb& box)>& fn) const;

private:
    [[nodiscard]] std::int64_t cell_key(int x, int y) const;

    float cell_size_ = 64.0f;
    std::unordered_map<std::int64_t, std::vector<entt::entity>> cells_;
};

} // namespace rivet::physics
