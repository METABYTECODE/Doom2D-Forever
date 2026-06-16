#include <d2df/ecs/game_world.hpp>

#include <d2df/map/map_spawn.hpp>

namespace d2df::ecs {

void GameWorld::reset_player(const map::MapDocument& map) {
    if (player_entity_ == entt::null) {
        player_entity_ = registry_.create();
        registry_.emplace<PlayerTag>(player_entity_);
        registry_.emplace<Transform>(player_entity_);
        registry_.emplace<Velocity>(player_entity_);
    }

    if (const auto spawn = map::find_player_spawn(map)) {
        player_.snap_to(static_cast<float>(spawn->x), static_cast<float>(spawn->y));
    } else {
        player_.snap_to(static_cast<float>(map.size.width) * 0.5f,
                        static_cast<float>(map.size.height) * 0.5f);
    }

    sync_to_ecs();
}

void GameWorld::fixed_update(const sim::MapCollision& collision, sim::PlayerInput input) {
    player_.fixed_update(collision, input);
    sync_to_ecs();
}

void GameWorld::sync_to_ecs() {
    if (player_entity_ == entt::null) {
        return;
    }

    auto& transform = registry_.get<Transform>(player_entity_);
    transform.prev_x = player_.prev_x;
    transform.prev_y = player_.prev_y;
    transform.x = player_.x;
    transform.y = player_.y;

    auto& velocity = registry_.get<Velocity>(player_entity_);
    velocity.x = player_.vel_x;
    velocity.y = player_.vel_y;
}

} // namespace d2df::ecs
