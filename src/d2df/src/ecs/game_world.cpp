#include <d2df/ecs/game_world.hpp>

#include <algorithm>

#include <d2df/core/event_bus.hpp>
#include <d2df/core/game_events.hpp>
#include <d2df/ecs/components/network_identity.hpp>
#include <d2df/map/map_spawn.hpp>
#include <d2df/map/monster_types.hpp>
#include <d2df/sim/combat_common.hpp>

namespace d2df::ecs {
namespace {

void publish_damage(EventBus* events, int amount, events::DamageSource source, int health_remaining) {
    if (events == nullptr || amount <= 0) {
        return;
    }
    events->publish(events::PlayerDamaged{amount, source, health_remaining});
    events->publish(events::EntityDamaged{kPlayerEntityId, 0, amount, source, health_remaining});
    if (health_remaining <= 0) {
        events->publish(events::PlayerDied{source});
        events->publish(events::EntityDied{kPlayerEntityId, source});
    }
}

} // namespace

void GameWorld::reset_player(const map::MapDocument& map) {
    if (player_entity_ == entt::null) {
        player_entity_ = registry_.create();
        registry_.emplace<PlayerTag>(player_entity_);
        registry_.emplace<Transform>(player_entity_);
        registry_.emplace<Velocity>(player_entity_);
        auto& identity = registry_.emplace<NetworkIdentity>(player_entity_);
        identity.net_id = kPlayerEntityId;
    }

    spawn_point_ = map::find_player_spawn(map);
    if (spawn_point_) {
        player_.reset_to_spawn(static_cast<float>(spawn_point_->x), static_cast<float>(spawn_point_->y));
    } else {
        player_.reset_to_spawn(static_cast<float>(map.size.width) * 0.5f,
                               static_cast<float>(map.size.height) * 0.5f);
    }

    targets_.clear();
    projectiles_.clear();
    items_.reset(map);
    spawn_map_monsters(map);
    if (targets_.empty()) {
        spawn_debug_target(map);
    }

    sync_to_ecs();
}

void GameWorld::spawn_map_monsters(const map::MapDocument& map) {
    next_monster_id_ = kDebugTargetBaseId;
    for (const auto& map_monster : map.monsters) {
        if (map_monster.type == map::MonsterType::None) {
            continue;
        }

        const auto stats = map::monster_stats(map_monster.type);
        sim::ShootableTarget target;
        target.id = next_monster_id_++;
        target.monster_type = map_monster.type;
        target.x = static_cast<float>(map_monster.position.x);
        target.y = static_cast<float>(map_monster.position.y);
        target.prev_x = target.x;
        target.prev_y = target.y;
        target.width = stats.width;
        target.height = stats.height;
        target.max_health = stats.health;
        target.health = stats.health;
        targets_.push_back(target);
    }
}

void GameWorld::spawn_debug_target(const map::MapDocument& map) {
    sim::ShootableTarget target;
    target.id = kDebugTargetBaseId;
    target.width = sim::PlayerState::width;
    target.height = sim::PlayerState::height;
    target.max_health = 100;
    target.health = target.max_health;

    const float base_x = spawn_point_ ? static_cast<float>(spawn_point_->x) : map.size.width * 0.5f;
    const float base_y = spawn_point_ ? static_cast<float>(spawn_point_->y) : map.size.height * 0.5f;
    target.x = base_x + 120.0f;
    target.y = base_y;

    targets_.push_back(target);
}

void GameWorld::respawn_player() {
    if (spawn_point_) {
        player_.reset_to_spawn(static_cast<float>(spawn_point_->x), static_cast<float>(spawn_point_->y));
    } else {
        player_.restore_health();
    }
    projectiles_.clear();
    sync_to_ecs();
}

void GameWorld::fixed_update(const sim::MapCollision& collision, sim::PlayerInput input,
                             EventBus* events, int map_width, int map_height,
                             sim::TriggerSystem* triggers) {
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

    monsters_.tick(collision, player_, targets_, events, &projectiles_, &next_monster_id_);

    weapons_.tick(player_, collision, input, targets_, triggers, &projectiles_, kPlayerEntityId,
                  events, map_width);

    projectiles_.tick(collision, triggers, player_, targets_, events, map_width, map_height);

    handle_monster_death_effects(collision, events, triggers);
    purge_dead_monsters();

    items_.tick(player_, &collision, events);

    sync_to_ecs();
}

void GameWorld::apply_environment_damage(const sim::MapCollision& collision, EventBus* events) {
    if (player_.has_suit()) {
        return;
    }

    if (player_.tick() % sim::PlayerState::kAcidDamagePeriod != 0) {
        return;
    }

    const int acid_damage =
        collision.acid_damage(player_.x, player_.y, player_.width, player_.height);
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

void GameWorld::purge_dead_monsters() {
    targets_.erase(std::remove_if(targets_.begin(), targets_.end(),
                                  [](const sim::ShootableTarget& target) {
                                      return target.is_monster() && !target.alive() &&
                                             map::monster_vanishes_on_death(target.monster_type);
                                  }),
                   targets_.end());
}

void GameWorld::handle_monster_death_effects(const sim::MapCollision& /*collision*/,
                                             EventBus* events, sim::TriggerSystem* triggers) {
    constexpr float kBarrelExplosionRadius = 60.0f;
    constexpr int kBarrelExplosionDamage = 100;

    bool processed_death = true;
    while (processed_death) {
        processed_death = false;
        for (auto& target : targets_) {
            if (!target.is_monster() || target.alive() || target.death_handled) {
                continue;
            }

            target.death_handled = true;
            processed_death = true;

            const float center_x = target.x + target.width * 0.5f;
            const float center_y = target.y + target.height * 0.5f;

            if (target.monster_type == map::MonsterType::Barrel) {
                sim::apply_area_explosion(center_x, center_y - 16.0f, kBarrelExplosionRadius,
                                          kBarrelExplosionDamage, player_, targets_, target.id,
                                          events, events::DamageSource::Projectile, triggers);
            } else if (target.monster_type == map::MonsterType::Pain) {
                for (int i = 0; i < 3; ++i) {
                    const float offset_x = static_cast<float>((i * 11) - 15);
                    const float offset_y = static_cast<float>((i * 5) - 7);
                    const auto stats = map::monster_stats(map::MonsterType::Soul);
                    sim::ShootableTarget soul;
                    soul.id = next_monster_id_++;
                    soul.monster_type = map::MonsterType::Soul;
                    soul.x = center_x + offset_x - stats.width * 0.5f;
                    soul.y = center_y + offset_y;
                    soul.prev_x = soul.x;
                    soul.prev_y = soul.y;
                    soul.width = stats.width;
                    soul.height = stats.height;
                    soul.max_health = stats.health;
                    soul.health = stats.health;
                    soul.aggro_player = true;
                    targets_.push_back(soul);
                }
            }
        }
    }
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
