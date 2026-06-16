#include <d2df/ecs/game_world.hpp>

#include <d2df/core/event_bus.hpp>
#include <d2df/core/game_events.hpp>
#include <d2df/map/map_spawn.hpp>

namespace d2df::ecs {
namespace {

void publish_damage(EventBus* events, int amount, events::DamageSource source, int health_remaining) {
    if (events == nullptr || amount <= 0) {
        return;
    }
    events->publish(events::PlayerDamaged{amount, source, health_remaining});
    if (health_remaining <= 0) {
        events->publish(events::PlayerDied{source});
    }
}

} // namespace

void GameWorld::reset_player(const map::MapDocument& map) {
    if (player_entity_ == entt::null) {
        player_entity_ = registry_.create();
        registry_.emplace<PlayerTag>(player_entity_);
        registry_.emplace<Transform>(player_entity_);
        registry_.emplace<Velocity>(player_entity_);
    }

    spawn_point_ = map::find_player_spawn(map);
    if (spawn_point_) {
        player_.snap_to(static_cast<float>(spawn_point_->x), static_cast<float>(spawn_point_->y));
    } else {
        player_.snap_to(static_cast<float>(map.size.width) * 0.5f,
                        static_cast<float>(map.size.height) * 0.5f);
    }

    sync_to_ecs();
}

void GameWorld::respawn_player() {
    if (spawn_point_) {
        player_.snap_to(static_cast<float>(spawn_point_->x), static_cast<float>(spawn_point_->y));
    } else {
        player_.restore_health();
    }
    sync_to_ecs();
}

void GameWorld::fixed_update(const sim::MapCollision& collision, sim::PlayerInput input,
                             EventBus* events) {
    if (!player_.alive()) {
        return;
    }

    const bool was_on_ground = player_.on_ground();
    const bool was_water = player_.in_water();
    const bool was_acid = player_.in_acid();

    player_.fixed_update(collision, input);

    publish_landed_event(was_on_ground, events);
    publish_liquid_events(was_water, was_acid, events);
    apply_environment_damage(collision, events);

    sync_to_ecs();
}

void GameWorld::apply_environment_damage(const sim::MapCollision& collision, EventBus* events) {
    if (player_.tick() % sim::PlayerState::kAcidDamagePeriod != 0) {
        return;
    }

    const int acid_damage = collision.acid_damage(player_.x, player_.y, player_.width, player_.height);
    if (acid_damage <= 0) {
        return;
    }

    const int health_before = player_.health();
    const bool died = player_.apply_damage(acid_damage);
    publish_damage(events, health_before - player_.health(), events::DamageSource::Acid,
                   player_.health());
    (void)died;
}

void GameWorld::publish_liquid_events(bool was_water, bool was_acid, EventBus* events) {
    if (events == nullptr) {
        return;
    }

    const bool now_water = player_.in_water();
    const bool now_acid = player_.in_acid();
    if ((!was_water && !was_acid) && (now_water || now_acid)) {
        events->publish(events::PlayerEnteredLiquid{now_water, now_acid});
    } else if ((was_water || was_acid) && !now_water && !now_acid) {
        events->publish(events::PlayerExitedLiquid{});
    }
}

void GameWorld::publish_landed_event(bool was_on_ground, EventBus* events) {
    if (events == nullptr || was_on_ground || !player_.on_ground()) {
        return;
    }
    events->publish(events::PlayerLanded{player_.x, player_.y});
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
