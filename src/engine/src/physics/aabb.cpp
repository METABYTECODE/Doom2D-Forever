#include <algorithm>

#include <rivet/physics/aabb.hpp>

namespace rivet::physics {

bool Aabb::intersects(const Aabb& other) const {
    return overlap_x(other) > 0.0f && overlap_y(other) > 0.0f;
}

float Aabb::overlap_x(const Aabb& other) const {
    return std::min(x + width, other.x + other.width) - std::max(x, other.x);
}

float Aabb::overlap_y(const Aabb& other) const {
    return std::min(y + height, other.y + other.height) - std::max(y, other.y);
}

Aabb make_aabb(const ecs::components::Transform& transform, const ecs::components::Collider& collider) {
    return Aabb{
        transform.x + collider.offset_x,
        transform.y + collider.offset_y,
        collider.width,
        collider.height,
    };
}

} // namespace rivet::physics
