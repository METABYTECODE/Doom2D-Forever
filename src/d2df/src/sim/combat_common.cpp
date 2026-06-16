#include <d2df/sim/combat_common.hpp>

#include <algorithm>
#include <cmath>

#include <d2df/core/event_bus.hpp>
#include <d2df/core/game_events.hpp>

namespace d2df::sim {

bool rects_overlap(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

bool segment_intersects_aabb(float x0, float y0, float x1, float y1, float bx, float by, float bw,
                             float bh, float& hit_x, float& hit_y, float& distance_sq) {
    const float dx = x1 - x0;
    const float dy = y1 - y0;
    float t0 = 0.0f;
    float t1 = 1.0f;

    const auto clip = [&](float p, float q) -> bool {
        if (p == 0.0f) {
            return q >= 0.0f;
        }
        const float r = q / p;
        if (p < 0.0f) {
            if (r > t1) {
                return false;
            }
            if (r > t0) {
                t0 = r;
            }
        } else {
            if (r < t0) {
                return false;
            }
            if (r < t1) {
                t1 = r;
            }
        }
        return true;
    };

    if (!clip(-dx, x0 - bx)) {
        return false;
    }
    if (!clip(dx, bx + bw - x0)) {
        return false;
    }
    if (!clip(-dy, y0 - by)) {
        return false;
    }
    if (!clip(dy, by + bh - y0)) {
        return false;
    }
    if (t0 > t1) {
        return false;
    }

    hit_x = x0 + dx * t0;
    hit_y = y0 + dy * t0;
    distance_sq = t0 * t0 * (dx * dx + dy * dy);
    return true;
}

bool segment_intersects_aabb(float x0, float y0, float x1, float y1, float bx, float by, float bw,
                             float bh) {
    float hit_x = 0.0f;
    float hit_y = 0.0f;
    float dist_sq = 0.0f;
    return segment_intersects_aabb(x0, y0, x1, y1, bx, by, bw, bh, hit_x, hit_y, dist_sq);
}

void publish_entity_damage(EventBus* events, EntityId target_id, EntityId source_id, int amount,
                           events::DamageSource source, int health_remaining) {
    if (events == nullptr || amount <= 0) {
        return;
    }
    events->publish(events::EntityDamaged{target_id, source_id, amount, source, health_remaining});
    if (health_remaining <= 0) {
        events->publish(events::EntityDied{target_id, source});
    }
}

void apply_damage_to_targets(std::vector<ShootableTarget>& targets, float cx, float cy, float radius,
                             int max_damage, EntityId source_id, EventBus* events,
                             events::DamageSource source) {
    if (radius <= 0.0f || max_damage <= 0) {
        return;
    }

    const float radius_sq = radius * radius;
    for (auto& target : targets) {
        if (!target.alive()) {
            continue;
        }

        const float tcx = target.x + target.width * 0.5f;
        const float tcy = target.y + target.height * 0.5f;
        const float dx = tcx - cx;
        const float dy = tcy - cy;
        const float dist_sq = dx * dx + dy * dy;
        if (dist_sq > radius_sq) {
            continue;
        }

        const float dist = std::sqrt(dist_sq);
        const int damage =
            static_cast<int>(static_cast<float>(max_damage) * (radius - dist) / radius);
        if (damage <= 0) {
            continue;
        }

        const int health_before = target.health;
        target.apply_damage(damage);
        publish_entity_damage(events, target.id, source_id, health_before - target.health, source,
                              target.health);
    }
}

bool damage_targets_along_ray(std::vector<ShootableTarget>& targets, float x0, float y0, float x1,
                              float y1, float wall_limit_sq, int damage, EntityId source_id,
                              EventBus* events, events::DamageSource source) {
    struct Candidate {
        std::size_t index = 0;
        float distance_sq = 0.0f;
    };

    std::vector<Candidate> candidates;
    for (std::size_t i = 0; i < targets.size(); ++i) {
        const auto& target = targets[i];
        if (!target.alive()) {
            continue;
        }
        float hit_x = 0.0f;
        float hit_y = 0.0f;
        float dist_sq = 0.0f;
        if (!segment_intersects_aabb(x0, y0, x1, y1, target.x, target.y, target.width, target.height,
                                     hit_x, hit_y, dist_sq)) {
            continue;
        }
        if (dist_sq >= wall_limit_sq) {
            continue;
        }
        candidates.push_back({i, dist_sq});
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& a, const Candidate& b) {
                  return a.distance_sq < b.distance_sq;
              });

    for (const auto& candidate : candidates) {
        auto& target = targets[candidate.index];
        const int health_before = target.health;
        target.apply_damage(damage);
        const int dealt = health_before - target.health;
        publish_entity_damage(events, target.id, source_id, dealt, source, target.health);
        if (dealt > 0) {
            return true;
        }
    }
    return false;
}

} // namespace d2df::sim
