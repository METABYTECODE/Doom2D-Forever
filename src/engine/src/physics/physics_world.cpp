#include <vector>

#include <entt/entt.hpp>

#include <rivet/ecs/components/collider.hpp>
#include <rivet/ecs/components/physics_contacts.hpp>
#include <rivet/ecs/components/transform.hpp>
#include <rivet/physics/aabb.hpp>
#include <rivet/physics/physics_world.hpp>
#include <rivet/physics/spatial_grid.hpp>

namespace rivet::physics {

namespace {

void resolve_axis_x(
    ecs::components::Transform& moving,
    ecs::components::Velocity& velocity,
    ecs::components::PhysicsContacts& contacts,
    const ecs::components::Collider& moving_collider,
    const Aabb& other_box) {
    const auto moving_box = make_aabb(moving, moving_collider);
    const float overlap_x = moving_box.overlap_x(other_box);
    const float overlap_y = moving_box.overlap_y(other_box);
    if (overlap_x <= 0.0f || overlap_y <= 0.0f) {
        return;
    }

    const float moving_center = moving_box.x + moving_box.width * 0.5f;
    const float other_center = other_box.x + other_box.width * 0.5f;
    const bool hit_from_right = moving_center < other_center;
    moving.x += hit_from_right ? -overlap_x : overlap_x;
    velocity.x = 0.0f;
    if (hit_from_right) {
        contacts.wall_right = true;
    } else {
        contacts.wall_left = true;
    }
}

void resolve_axis_y(
    ecs::components::Transform& moving,
    ecs::components::Velocity& velocity,
    ecs::components::PhysicsContacts& contacts,
    const ecs::components::Collider& moving_collider,
    const Aabb& other_box) {
    const auto moving_box = make_aabb(moving, moving_collider);
    const float overlap_x = moving_box.overlap_x(other_box);
    const float overlap_y = moving_box.overlap_y(other_box);
    if (overlap_x <= 0.0f || overlap_y <= 0.0f) {
        return;
    }

    const float moving_center = moving_box.y + moving_box.height * 0.5f;
    const float other_center = other_box.y + other_box.height * 0.5f;
    const bool push_up = moving_center < other_center;
    moving.y += push_up ? -overlap_y : overlap_y;

    if (push_up && velocity.y > 0.0f) {
        velocity.y = 0.0f;
        contacts.floor = true;
    } else if (!push_up && velocity.y < 0.0f) {
        velocity.y = 0.0f;
        contacts.ceiling = true;
    }
}

void resolve_bounds(
    ecs::components::Transform& transform,
    ecs::components::Velocity& velocity,
    ecs::components::PhysicsContacts& contacts,
    const ecs::components::Collider& collider,
    const PhysicsWorld::Bounds& bounds) {
    if (!bounds.enabled) {
        return;
    }

    auto box = make_aabb(transform, collider);

    if (box.x < bounds.min_x) {
        transform.x += bounds.min_x - box.x;
        velocity.x = 0.0f;
        contacts.wall_left = true;
        box = make_aabb(transform, collider);
    }
    if (box.y < bounds.min_y) {
        transform.y += bounds.min_y - box.y;
        velocity.y = 0.0f;
        contacts.ceiling = true;
        box = make_aabb(transform, collider);
    }
    if (box.x + box.width > bounds.max_x) {
        transform.x -= (box.x + box.width) - bounds.max_x;
        velocity.x = 0.0f;
        contacts.wall_right = true;
        box = make_aabb(transform, collider);
    }
    if (box.y + box.height > bounds.max_y) {
        transform.y -= (box.y + box.height) - bounds.max_y;
        velocity.y = 0.0f;
        contacts.floor = true;
    }
}

void ensure_contacts(ecs::World& world, const entt::entity entity) {
    if (!world.registry().all_of<ecs::components::PhysicsContacts>(entity)) {
        world.registry().emplace<ecs::components::PhysicsContacts>(entity);
    } else {
        world.registry().get<ecs::components::PhysicsContacts>(entity) = {};
    }
}

void resolve_tiles_x(
    ecs::components::Transform& transform,
    ecs::components::Velocity& velocity,
    ecs::components::PhysicsContacts& contacts,
    const ecs::components::Collider& collider,
    const TileCollider& tiles) {
    const Aabb body = make_aabb(transform, collider);
    tiles.for_each_cell_in_aabb(body, [&](int, int, const Aabb& cell) {
        resolve_axis_x(transform, velocity, contacts, collider, cell);
    });
}

void resolve_tiles_y(
    ecs::components::Transform& transform,
    ecs::components::Velocity& velocity,
    ecs::components::PhysicsContacts& contacts,
    const ecs::components::Collider& collider,
    const TileCollider& tiles) {
    const Aabb body = make_aabb(transform, collider);
    tiles.for_each_cell_in_aabb(body, [&](int, int, const Aabb& cell) {
        resolve_axis_y(transform, velocity, contacts, collider, cell);
    });
}

void resolve_statics_x(
    ecs::components::Transform& transform,
    ecs::components::Velocity& velocity,
    ecs::components::PhysicsContacts& contacts,
    const ecs::components::Collider& collider,
    const SpatialGrid& statics,
    const ecs::World& world) {
    const Aabb body = make_aabb(transform, collider);
    statics.for_each_in_aabb(body, [&](const entt::entity entity, const Aabb&) {
        const auto& other_transform = world.registry().get<ecs::components::Transform>(entity);
        const auto& other_collider = world.registry().get<ecs::components::Collider>(entity);
        resolve_axis_x(transform, velocity, contacts, collider, make_aabb(other_transform, other_collider));
    });
}

void resolve_statics_y(
    ecs::components::Transform& transform,
    ecs::components::Velocity& velocity,
    ecs::components::PhysicsContacts& contacts,
    const ecs::components::Collider& collider,
    const SpatialGrid& statics,
    const ecs::World& world) {
    const Aabb body = make_aabb(transform, collider);
    statics.for_each_in_aabb(body, [&](const entt::entity entity, const Aabb&) {
        const auto& other_transform = world.registry().get<ecs::components::Transform>(entity);
        const auto& other_collider = world.registry().get<ecs::components::Collider>(entity);
        resolve_axis_y(transform, velocity, contacts, collider, make_aabb(other_transform, other_collider));
    });
}

} // namespace

void PhysicsWorld::set_world_bounds(const Bounds& bounds) {
    bounds_ = bounds;
}

void PhysicsWorld::set_tile_collider(TileCollider collider) {
    tiles_ = std::move(collider);
}

void PhysicsWorld::set_broadphase_cell_size(const float cell_size) {
    broadphase_cell_size_ = std::max(8.0f, cell_size);
}

void PhysicsWorld::step(ecs::World& world, const float fixed_delta_time) {
    SpatialGrid statics;
    statics.build(world, broadphase_cell_size_);

    auto dynamic_view = world.registry().view<
        ecs::components::Transform,
        ecs::components::Velocity,
        ecs::components::Collider>();

    std::vector<entt::entity> dynamics;
    for (const auto entity : dynamic_view) {
        const auto& collider = dynamic_view.get<ecs::components::Collider>(entity);
        if (collider.is_static) {
            continue;
        }
        ensure_contacts(world, entity);
        dynamics.push_back(entity);
    }

    for (const auto entity : dynamics) {
        auto& transform = dynamic_view.get<ecs::components::Transform>(entity);
        auto& velocity = dynamic_view.get<ecs::components::Velocity>(entity);
        auto& contacts = world.registry().get<ecs::components::PhysicsContacts>(entity);
        const auto& collider = dynamic_view.get<ecs::components::Collider>(entity);

        transform.x += velocity.x * fixed_delta_time;
        if (!tiles_.empty()) {
            resolve_tiles_x(transform, velocity, contacts, collider, tiles_);
        }
        resolve_statics_x(transform, velocity, contacts, collider, statics, world);
        resolve_bounds(transform, velocity, contacts, collider, bounds_);

        transform.y += velocity.y * fixed_delta_time;
        if (!tiles_.empty()) {
            resolve_tiles_y(transform, velocity, contacts, collider, tiles_);
        }
        resolve_statics_y(transform, velocity, contacts, collider, statics, world);
        resolve_bounds(transform, velocity, contacts, collider, bounds_);
    }

    for (std::size_t i = 0; i < dynamics.size(); ++i) {
        for (std::size_t j = i + 1; j < dynamics.size(); ++j) {
            auto& transform_a = dynamic_view.get<ecs::components::Transform>(dynamics[i]);
            auto& velocity_a = dynamic_view.get<ecs::components::Velocity>(dynamics[i]);
            auto& contacts_a = world.registry().get<ecs::components::PhysicsContacts>(dynamics[i]);
            const auto& collider_a = dynamic_view.get<ecs::components::Collider>(dynamics[i]);
            auto& transform_b = dynamic_view.get<ecs::components::Transform>(dynamics[j]);
            auto& velocity_b = dynamic_view.get<ecs::components::Velocity>(dynamics[j]);
            auto& contacts_b = world.registry().get<ecs::components::PhysicsContacts>(dynamics[j]);
            const auto& collider_b = dynamic_view.get<ecs::components::Collider>(dynamics[j]);

            const Aabb box_b = make_aabb(transform_b, collider_b);
            resolve_axis_x(transform_a, velocity_a, contacts_a, collider_a, box_b);
            const Aabb box_a = make_aabb(transform_a, collider_a);
            resolve_axis_x(transform_b, velocity_b, contacts_b, collider_b, box_a);
            resolve_axis_y(transform_a, velocity_a, contacts_a, collider_a, make_aabb(transform_b, collider_b));
            resolve_axis_y(transform_b, velocity_b, contacts_b, collider_b, make_aabb(transform_a, collider_a));
        }
    }
}

void PhysicsWorld::clear() {
    bounds_ = {};
    tiles_ = {};
    broadphase_cell_size_ = 64.0f;
}

} // namespace rivet::physics
