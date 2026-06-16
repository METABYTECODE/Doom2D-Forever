#pragma once

#include <d2df/core/types.hpp>
#include <d2df/sim/shootable_target.hpp>

#include <vector>

namespace d2df {

class EventBus;

} // namespace d2df

namespace d2df::events {
enum class DamageSource : std::uint8_t;
}

namespace d2df::sim {

struct PlayerState;
class TriggerSystem;

/// Liang-Barsky segment vs AABB; returns entry point along segment [0,1].
bool segment_intersects_aabb(float x0, float y0, float x1, float y1, float bx, float by, float bw,
                             float bh, float& hit_x, float& hit_y, float& distance_sq);

bool segment_intersects_aabb(float x0, float y0, float x1, float y1, float bx, float by, float bw,
                             float bh);

bool rects_overlap(float ax, float ay, float aw, float ah, float bx, float by, float bw,
                     float bh);

void publish_entity_damage(EventBus* events, EntityId target_id, EntityId source_id, int amount,
                           events::DamageSource source, int health_remaining);

void apply_damage_to_targets(std::vector<ShootableTarget>& targets, float cx, float cy, float radius,
                             int max_damage, EntityId source_id, EventBus* events,
                             events::DamageSource source);

/// Pascal-style explosion (Chebyshev falloff). Damages player + monsters, optional trigger press.
void apply_area_explosion(float cx, float cy, float radius, int max_player_damage,
                          PlayerState& player, std::vector<ShootableTarget>& targets,
                          EntityId source_id, EventBus* events, events::DamageSource source,
                          TriggerSystem* triggers = nullptr);

bool damage_targets_along_ray(std::vector<ShootableTarget>& targets, float x0, float y0, float x1,
                              float y1, float wall_limit_sq, int damage, EntityId source_id,
                              EventBus* events, events::DamageSource source);

} // namespace d2df::sim
