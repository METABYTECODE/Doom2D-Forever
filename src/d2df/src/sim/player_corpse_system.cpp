#include <d2df/sim/player_corpse_system.hpp>

#include <algorithm>
#include <cmath>

#include <d2df/sim/map_collision.hpp>
#include <d2df/sim/player_state.hpp>
#include <d2df/sim/player_types.hpp>

namespace d2df::sim {
namespace {

int z_dec(int value, int amount) {
    if (value > 0) {
        return std::max(0, value - amount);
    }
    if (value < 0) {
        return std::min(0, value + amount);
    }
    return 0;
}

int pseudo_random(int seed, int min_value, int max_value) {
    const int span = max_value - min_value + 1;
    if (span <= 1) {
        return min_value;
    }
    const int mixed = (seed * 1103515245 + 12345) & 0x7fffffff;
    return min_value + (mixed % span);
}

bool is_below_map(float y, int map_height) {
    return y >= static_cast<float>(map_height + kPlayerCorpseFalloutBelowMap);
}

} // namespace

void PlayerCorpseSystem::clear() {
    corpses_.clear();
    gibs_.clear();
}

void PlayerCorpseSystem::spawn_from_death(const PlayerState& player, int map_height) {
    if (player.corpse_resolved()) {
        return;
    }

    if (player.death_health() < kPlayerDeathGibHealthThreshold) {
        const float center_x = player.x + PlayerState::width * 0.5f;
        const float center_y = player.y + PlayerState::height * 0.5f;
        spawn_gibs(center_x, center_y, player.vel_x / 2, player.vel_y / 2);
        return;
    }

    if (is_below_map(player.y, map_height)) {
        return;
    }

    spawn_corpse(player);
}

void PlayerCorpseSystem::spawn_corpse(const PlayerState& player) {
    PlayerCorpse corpse;
    corpse.x = player.x;
    corpse.y = player.y;
    corpse.prev_x = player.x;
    corpse.prev_y = player.y;
    corpse.vel_x = player.vel_x / 2;
    corpse.vel_y = player.vel_y / 2;
    corpse.facing_left = player.combat().facing_left;
    corpse.mess = player.death_health() < kPlayerDeathMessHealthThreshold;
    corpses_.push_back(corpse);
}

void PlayerCorpseSystem::spawn_gibs(float center_x, float center_y, int base_vel_x, int base_vel_y) {
    for (int i = 0; i < kPlayerGibCount; ++i) {
        PlayerGib gib;
        gib.src_frame = i;
        gib.rotation_deg = pseudo_random(i * 17 + static_cast<int>(center_x), 0, 359);
        gib.x = center_x - static_cast<float>(kPlayerGibFrameSize) * 0.5f +
                static_cast<float>(pseudo_random(i * 3, -8, 8));
        gib.y = center_y - static_cast<float>(kPlayerGibFrameSize) * 0.5f +
                static_cast<float>(pseudo_random(i * 5, -8, 8));
        gib.prev_x = gib.x;
        gib.prev_y = gib.y;

        const int speed = pseudo_random(i * 11 + static_cast<int>(center_y), 25, 34);
        const int angle_deg = pseudo_random(i * 13, 0, 359);
        const double angle_rad = angle_deg * 3.141592653589793 / 180.0;
        gib.vel_x = base_vel_x + static_cast<int>(std::lround(std::cos(angle_rad) * speed));
        gib.vel_y = base_vel_y + static_cast<int>(std::lround(std::sin(angle_rad) * speed));
        gibs_.push_back(gib);
    }
}

void PlayerCorpseSystem::tick(const MapCollision& collision, int map_height) {
    for (auto& corpse : corpses_) {
        if (corpse.active) {
            tick_corpse(corpse, collision, map_height);
        }
    }
    corpses_.erase(std::remove_if(corpses_.begin(), corpses_.end(),
                                  [](const PlayerCorpse& corpse) { return !corpse.active; }),
                   corpses_.end());

    for (auto& gib : gibs_) {
        if (gib.active) {
            tick_gib(gib, collision, map_height);
        }
    }
    gibs_.erase(std::remove_if(gibs_.begin(), gibs_.end(),
                               [](const PlayerGib& gib) { return !gib.active; }),
                gibs_.end());
}

void PlayerCorpseSystem::tick_corpse(PlayerCorpse& corpse, const MapCollision& collision,
                                     int map_height) {
    corpse.prev_x = corpse.x;
    corpse.prev_y = corpse.y;
    ++corpse.tick;

    if (is_below_map(corpse.y, map_height)) {
        corpse.active = false;
        return;
    }

    if ((corpse.tick % 2) == 0) {
        corpse.vel_y += 1;
        if (corpse.vel_y > PlayerState::kMaxYVel) {
            corpse.vel_y = PlayerState::kMaxYVel;
        }
    }

    if ((corpse.tick % 2) == 1) {
        corpse.vel_x = z_dec(corpse.vel_x, 1);
    }

    float x = corpse.x;
    float y = corpse.y;
    float hitbox_x = x + kPlayerCorpseHitboxOffsetX;
    float hitbox_y = y + kPlayerCorpseHitboxOffsetY;
    const auto state = collision.move_object(hitbox_x, hitbox_y, kPlayerCorpseHitboxWidth,
                                             kPlayerCorpseHitboxHeight, corpse.vel_x, corpse.vel_y,
                                             true);
    corpse.x = hitbox_x - kPlayerCorpseHitboxOffsetX;
    corpse.y = hitbox_y - kPlayerCorpseHitboxOffsetY;

    if ((state & MOVE_HITLAND) != 0 || (state & MOVE_HITCEIL) != 0) {
        corpse.vel_y = 0;
    }
    if ((state & MOVE_HITWALL) != 0) {
        corpse.vel_x = -(corpse.vel_x / 2);
    }

    if (is_below_map(corpse.y, map_height)) {
        corpse.active = false;
    }
}

void PlayerCorpseSystem::tick_gib(PlayerGib& gib, const MapCollision& collision, int map_height) {
    gib.prev_x = gib.x;
    gib.prev_y = gib.y;
    ++gib.tick;

    if (is_below_map(gib.y, map_height)) {
        gib.active = false;
        return;
    }

    gib.vel_y += 1;
    if (gib.vel_y > PlayerState::kMaxYVel) {
        gib.vel_y = PlayerState::kMaxYVel;
    }

    if ((gib.tick % 2) == 0) {
        gib.vel_x = z_dec(gib.vel_x, 1);
    }

    float x = gib.x;
    float y = gib.y;
    const auto state = collision.move_object(
        x, y, static_cast<float>(kPlayerGibFrameSize), static_cast<float>(kPlayerGibFrameSize),
        gib.vel_x, gib.vel_y, true);
    gib.x = x;
    gib.y = y;

    if ((state & MOVE_HITLAND) != 0 || (state & MOVE_HITCEIL) != 0) {
        gib.vel_y = -(gib.vel_y / 2);
    }
    if ((state & MOVE_HITWALL) != 0) {
        gib.vel_x = -(gib.vel_x / 2);
    }

    gib.rotation_deg = (gib.rotation_deg + gib.vel_x + 4) % 360;

    if (is_below_map(gib.y, map_height)) {
        gib.active = false;
    }
}

} // namespace d2df::sim
