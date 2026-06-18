#include <cmath>
#include <vector>

#include <entt/entt.hpp>

#include <rivet/ecs/components/collider.hpp>
#include <rivet/ecs/components/transform.hpp>
#include <rivet/physics/aabb.hpp>
#include <rivet/physics/physics_world.hpp>

namespace rivet::physics {

namespace {

void integrate(ecs::World& world, const float dt) {
    auto view = world.registry().view<ecs::components::Transform, ecs::components::Velocity>();
    for (const auto entity : view) {
        auto& transform = view.get<ecs::components::Transform>(entity);
        const auto& velocity = view.get<ecs::components::Velocity>(entity);
        transform.x += velocity.x * dt;
        transform.y += velocity.y * dt;
    }
}

void resolve_pair(
    ecs::components::Transform& moving,
  ecs::components::Collider& moving_collider,
    const ecs::components::Transform& other,
    const ecs::components::Collider& other_collider) {
    const auto moving_box = make_aabb(moving, moving_collider);
    const auto other_box = make_aabb(other, other_collider);
    if (!moving_box.intersects(other_box)) {
        return;
    }

    const float overlap_x = moving_box.overlap_x(other_box);
    const float overlap_y = moving_box.overlap_y(other_box);
    if (overlap_x <= 0.0f || overlap_y <= 0.0f) {
        return;
    }

    if (overlap_x < overlap_y) {
        const float moving_center = moving_box.x + moving_box.width * 0.5f;
        const float other_center = other_box.x + other_box.width * 0.5f;
        moving.x += (moving_center < other_center ? -overlap_x : overlap_x);
    } else {
        const float moving_center = moving_box.y + moving_box.height * 0.5f;
        const float other_center = other_box.y + other_box.height * 0.5f;
        moving.y += (moving_center < other_center ? -overlap_y : overlap_y);
    }
}

void resolve_bounds(
    ecs::components::Transform& transform,
    ecs::components::Collider& collider,
    ecs::components::Velocity& velocity,
    const PhysicsWorld::Bounds& bounds) {
    auto box = make_aabb(transform, collider);

    if (box.x < bounds.min_x) {
        transform.x += bounds.min_x - box.x;
        velocity.x = std::abs(velocity.x);
        box = make_aabb(transform, collider);
    }
    if (box.y < bounds.min_y) {
        transform.y += bounds.min_y - box.y;
        velocity.y = std::abs(velocity.y);
        box = make_aabb(transform, collider);
    }
    if (box.x + box.width > bounds.max_x) {
        transform.x -= (box.x + box.width) - bounds.max_x;
        velocity.x = -std::abs(velocity.x);
    }
    if (box.y + box.height > bounds.max_y) {
        transform.y -= (box.y + box.height) - bounds.max_y;
        velocity.y = -std::abs(velocity.y);
    }
}

} // namespace

void PhysicsWorld::set_world_bounds(const Bounds& bounds) {
    bounds_ = bounds;
}

void PhysicsWorld::step(ecs::World& world, const float fixed_delta_time) {
    integrate(world, fixed_delta_time);

    auto dynamic_view =
        world.registry().view<ecs::components::Transform, ecs::components::Velocity, ecs::components::Collider>();
    const auto static_view =
        world.registry().view<ecs::components::Transform, ecs::components::Collider>();

    for (const auto entity : dynamic_view) {
        auto& transform = dynamic_view.get<ecs::components::Transform>(entity);
        auto& velocity = dynamic_view.get<ecs::components::Velocity>(entity);
        auto& collider = dynamic_view.get<ecs::components::Collider>(entity);
        if (collider.is_static) {
            continue;
        }

        if (bounds_.enabled) {
            resolve_bounds(transform, collider, velocity, bounds_);
        }

        for (const auto other_entity : static_view) {
            if (entity == other_entity) {
                continue;
            }
            const auto& other_collider = static_view.get<ecs::components::Collider>(other_entity);
            if (!other_collider.is_static) {
                continue;
            }
            const auto& other_transform = static_view.get<ecs::components::Transform>(other_entity);
            resolve_pair(transform, collider, other_transform, other_collider);
        }
    }

    std::vector<entt::entity> dynamics;
    for (const auto entity : dynamic_view) {
        const auto& collider = dynamic_view.get<ecs::components::Collider>(entity);
        if (!collider.is_static) {
            dynamics.push_back(entity);
        }
    }

    for (std::size_t i = 0; i < dynamics.size(); ++i) {
        for (std::size_t j = i + 1; j < dynamics.size(); ++j) {
            auto& transform_a = dynamic_view.get<ecs::components::Transform>(dynamics[i]);
            auto& collider_a = dynamic_view.get<ecs::components::Collider>(dynamics[i]);
            auto& transform_b = dynamic_view.get<ecs::components::Transform>(dynamics[j]);
            auto& collider_b = dynamic_view.get<ecs::components::Collider>(dynamics[j]);

            resolve_pair(transform_a, collider_a, transform_b, collider_b);
            resolve_pair(transform_b, collider_b, transform_a, collider_a);
        }
    }
}

void PhysicsWorld::clear() {
    bounds_ = {};
}

} // namespace rivet::physics
