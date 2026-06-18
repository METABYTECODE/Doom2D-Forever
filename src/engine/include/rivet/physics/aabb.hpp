#pragma once

#include <rivet/ecs/components/collider.hpp>
#include <rivet/ecs/components/transform.hpp>

namespace rivet::physics {

struct Aabb {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    [[nodiscard]] bool intersects(const Aabb& other) const;
    [[nodiscard]] float overlap_x(const Aabb& other) const;
    [[nodiscard]] float overlap_y(const Aabb& other) const;
};

[[nodiscard]] Aabb make_aabb(const ecs::components::Transform& transform, const ecs::components::Collider& collider);

} // namespace rivet::physics
