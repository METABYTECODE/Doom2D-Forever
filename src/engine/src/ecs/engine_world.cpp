#include <d2df/engine/ecs/engine_world.hpp>

#include <d2df/engine/map/legacy_map_adapter.hpp>
#include <d2df/engine/physics/entity_collision.hpp>

namespace d2df::engine {
namespace {

sim::PlayerInput to_player_input(MovementInput input) {
    sim::PlayerInput out;
    out.left = input.left;
    out.right = input.right;
    out.jump = input.jump;
    return out;
}

} // namespace

void EngineWorld::ensure_player_entity() {
    if (player_entity_ != entt::null) {
        return;
    }

    player_entity_ = registry_.create();
    registry_.emplace<::d2df::ecs::Transform>(player_entity_);
    registry_.emplace<::d2df::ecs::Velocity>(player_entity_);
    registry_.emplace<::d2df::ecs::PlayerTag>(player_entity_);
}

void EngineWorld::clear_entities() {
    for (const auto entity : entity_entities_) {
        registry_.destroy(entity);
    }
    entity_entities_.clear();
}

void EngineWorld::spawn_entities_from_map(const map::TileMap& tile_map) {
    clear_entities();

    for (const auto& map_entity : tile_map.entities) {
        if (map_entity.type.empty()) {
            continue;
        }

        const auto entity = registry_.create();

        auto& transform = registry_.emplace<::d2df::ecs::Transform>(entity);
        transform.x = static_cast<float>(map_entity.x);
        transform.y = static_cast<float>(map_entity.y);
        transform.prev_x = transform.x;
        transform.prev_y = transform.y;

        auto& collider = registry_.emplace<ecs::Collider>(entity);
        collider.width = map_entity.width;
        collider.height = map_entity.height;
        collider.solid = true;

        registry_.emplace<ecs::MonsterTag>(entity);
        entity_entities_.push_back(entity);
    }
}

void EngineWorld::sync_player_to_ecs() {
    if (player_entity_ == entt::null) {
        return;
    }

    auto& transform = registry_.get<::d2df::ecs::Transform>(player_entity_);
    transform.prev_x = player_.prev_x;
    transform.prev_y = player_.prev_y;
    transform.x = player_.x;
    transform.y = player_.y;

    auto& velocity = registry_.get<::d2df::ecs::Velocity>(player_entity_);
    velocity.x = player_.vel_x;
    velocity.y = player_.vel_y;
}

void EngineWorld::resolve_entity_collisions() {
    const auto& registry = registry_;
    for (const auto entity : entity_entities_) {
        if (!registry.all_of<ecs::Collider, ::d2df::ecs::Transform>(entity)) {
            continue;
        }

        const auto& transform = registry.get<::d2df::ecs::Transform>(entity);
        const auto& collider = registry.get<ecs::Collider>(entity);
        if (!collider.solid) {
            continue;
        }

        physics::resolve_player_entity_collisions(player_, transform.x, transform.y, collider.width,
                                                  collider.height);
    }
}

void EngineWorld::reset(const map::TileMap& tile_map) {
    ensure_player_entity();

    const auto spawn = map::find_spawn(tile_map);
    if (spawn) {
        player_.reset_to_spawn(static_cast<float>(spawn->x), static_cast<float>(spawn->y));
    } else if (tile_map.width > 0 && tile_map.height > 0) {
        player_.reset_to_spawn(static_cast<float>(tile_map.width) * 0.5f,
                               static_cast<float>(tile_map.height) * 0.5f);
    } else {
        player_.reset_to_spawn(0.0f, 0.0f);
    }

    spawn_entities_from_map(tile_map);
    sync_player_to_ecs();
}

void EngineWorld::fixed_tick(physics::CollisionWorld& collision, MovementInput input) {
    player_.begin_tick();
    player_.fixed_update(collision.collision(), to_player_input(input));
    resolve_entity_collisions();
    sync_player_to_ecs();
}

} // namespace d2df::engine
